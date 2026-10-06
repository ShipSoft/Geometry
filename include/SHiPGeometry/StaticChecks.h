// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

#include <cstddef>
#include <limits>

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
namespace SHiPGeometry::checks {

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

}  // namespace SHiPGeometry::checks
