// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/Units.h"

#include <array>
#include <cstddef>
#include <limits>
#include <utility>

/**
 * @brief Constexpr predicates for compile-time geometry consistency checks.
 *
 * The geometry constants are all constexpr quantities, so containment,
 * ordering and non-overlap can be checked with static_assert instead of
 * building volumes in a test. The predicates are plain constexpr functions
 * so tests can also feed them bad inputs as negative controls.
 *
 * Only comparisons and +/- are used: no <cmath>, whose functions Clang does
 * not fold in constant expressions (clang-tidy parses the tree with Clang).
 */
namespace SHiP::geometry::checks {

using units::LengthMm;

/// Closed interval along one axis.
struct Span {
    LengthMm lo;
    LengthMm hi;
};

/// Interval of a volume with the given centre and half-length.
[[nodiscard]] constexpr Span span(LengthMm centre, LengthMm half) {
    return {centre - half, centre + half};
}

/// @p inner lies within @p outer, allowing @p tol of slack at either end.
[[nodiscard]] constexpr bool contains(Span outer, Span inner, LengthMm tol = 0.0 * units::mm) {
    return outer.lo - tol <= inner.lo && inner.hi <= outer.hi + tol;
}

/// The intervals share more than a boundary. Touching ends do not overlap.
[[nodiscard]] constexpr bool overlaps(Span a, Span b) {
    return a.lo < b.hi && b.lo < a.hi;
}

/// Axis-aligned box.
struct Box {
    Span x;
    Span y;
    Span z;
};

/// Box with centre (@p cx, @p cy, @p cz) and the given half-lengths.
[[nodiscard]] constexpr Box box(LengthMm cx, LengthMm cy, LengthMm cz, LengthMm hx, LengthMm hy,
                                LengthMm hz) {
    return {span(cx, hx), span(cy, hy), span(cz, hz)};
}

/// Box centred on the origin.
[[nodiscard]] constexpr Box box(LengthMm hx, LengthMm hy, LengthMm hz) {
    constexpr auto zero = 0.0 * units::mm;
    return box(zero, zero, zero, hx, hy, hz);
}

[[nodiscard]] constexpr bool contains(const Box& outer, const Box& inner,
                                      LengthMm tol = 0.0 * units::mm) {
    return contains(outer.x, inner.x, tol) && contains(outer.y, inner.y, tol) &&
           contains(outer.z, inner.z, tol);
}

/// Boxes overlap only if they overlap along every axis.
[[nodiscard]] constexpr bool overlaps(const Box& a, const Box& b) {
    return overlaps(a.x, b.x) && overlaps(a.y, b.y) && overlaps(a.z, b.z);
}

/// Box from its bounds in mm, for writing a known region down literally.
[[nodiscard]] constexpr Box bounds(double x0, double x1, double y0, double y1, double z0,
                                   double z1) {
    return {.x = {.lo = x0 * units::mm, .hi = x1 * units::mm},
            .y = {.lo = y0 * units::mm, .hi = y1 * units::mm},
            .z = {.lo = z0 * units::mm, .hi = z1 * units::mm}};
}

/// The region two boxes share. Only meaningful if they overlap.
[[nodiscard]] constexpr Box intersection(const Box& a, const Box& b) {
    const auto meet = [](Span s, Span t) {
        return Span{.lo = s.lo < t.lo ? t.lo : s.lo, .hi = s.hi < t.hi ? s.hi : t.hi};
    };
    return {.x = meet(a.x, b.x), .y = meet(a.y, b.y), .z = meet(a.z, b.z)};
}

/// The boxes overlap, and only within @p region. Bounds a known overlap: it
/// fails both when the overlap is removed and when it grows.
[[nodiscard]] constexpr bool overlapsWithin(const Box& a, const Box& b, const Box& region,
                                            LengthMm tol = 1e-6 * units::mm) {
    return overlaps(a, b) && contains(region, intersection(a, b), tol);
}

/// @p b with every face moved inwards by @p d. A box sticks out of a region
/// by at most d exactly when shrunk(box, d) lies within it.
[[nodiscard]] constexpr Box shrunk(const Box& b, LengthMm d) {
    return {.x = {.lo = b.x.lo + d, .hi = b.x.hi - d},
            .y = {.lo = b.y.lo + d, .hi = b.y.hi - d},
            .z = {.lo = b.z.lo + d, .hi = b.z.hi - d}};
}

/// Returned by the first*() searches when every element passes.
inline constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

/**
 * Index of the first element of @p range for which @p ok is false, or npos.
 *
 * Asserting `firstFailure(...) == npos` instead of a plain all-of makes the
 * compiler print the offending index when the assertion fails.
 */
template <typename Range, typename Pred>
[[nodiscard]] constexpr std::size_t firstFailure(const Range& range, Pred ok) {
    std::size_t i = 0;
    for (const auto& element : range) {
        if (!ok(element)) {
            return i;
        }
        ++i;
    }
    return npos;
}

/// Index i of the first neighbouring pair (range[i], range[i+1]) for which
/// @p ok is false, or npos.
template <typename Range, typename Pred>
[[nodiscard]] constexpr std::size_t firstPairFailure(const Range& range, Pred ok) {
    for (std::size_t i = 0; i + 1 < std::size(range); ++i) {
        if (!ok(range[i], range[i + 1])) {
            return i;
        }
    }
    return npos;
}

/**
 * Whether @p inner lies within the union of @p outers.
 *
 * Every bound of an outer box that falls inside @p inner cuts it into cells;
 * each cell lies either wholly inside or wholly outside each outer box, so
 * testing the cell centres decides coverage exactly.
 */
template <std::size_t N>
[[nodiscard]] constexpr bool coveredBy(const Box& inner, const std::array<Box, N>& outers) {
    struct Cuts {
        std::array<LengthMm, 2 * N + 2> at{};
        std::size_t n = 0;
    };
    const auto cuts = [&](Span Box::* axis) {
        const Span s = inner.*axis;
        Cuts c;
        c.at[c.n++] = s.lo;
        c.at[c.n++] = s.hi;
        for (const auto& outer : outers) {
            for (const LengthMm v : {(outer.*axis).lo, (outer.*axis).hi}) {
                if (s.lo < v && v < s.hi) {
                    c.at[c.n++] = v;
                }
            }
        }
        for (std::size_t i = 1; i < c.n; ++i) {
            for (std::size_t j = i; j > 0 && c.at[j] < c.at[j - 1]; --j) {
                std::swap(c.at[j], c.at[j - 1]);
            }
        }
        return c;
    };
    const auto inside = [](Span s, LengthMm v) { return s.lo <= v && v <= s.hi; };

    const Cuts cx = cuts(&Box::x);
    const Cuts cy = cuts(&Box::y);
    const Cuts cz = cuts(&Box::z);
    for (std::size_t i = 0; i + 1 < cx.n; ++i) {
        for (std::size_t j = 0; j + 1 < cy.n; ++j) {
            for (std::size_t k = 0; k + 1 < cz.n; ++k) {
                const LengthMm x = (cx.at[i] + cx.at[i + 1]) / 2.0;
                const LengthMm y = (cy.at[j] + cy.at[j + 1]) / 2.0;
                const LengthMm z = (cz.at[k] + cz.at[k + 1]) / 2.0;
                bool covered = false;
                for (const auto& outer : outers) {
                    covered =
                        covered || (inside(outer.x, x) && inside(outer.y, y) && inside(outer.z, z));
                }
                if (!covered) {
                    return false;
                }
            }
        }
    }
    return true;
}

}  // namespace SHiP::geometry::checks
