// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "DecayVolume/SBTConstants.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

class GeoMaterial;
class GeoPhysVol;

namespace SHiPGeometry::SBT {

/**
 * @brief The innermost free region of the SBT, and the helium that fills it.
 *
 * The helium decay region must fill the space inside the SBT exactly: it may
 * not protrude into any steel or scintillator, and it may not leave an
 * arbitrary safety margin behind either. That makes its size a *consequence*
 * of where the SBT material is, never an independent parameter.
 *
 * This header therefore derives the helium purely from the placement
 * primitives in SBTConstants.h — the same ones SBTStructureBuilder and
 * SBTSensorBuilder place their volumes with. Change a placement rule and the
 * helium follows automatically; there is no second copy of the arithmetic to
 * forget to update. The derivation is constexpr, so an SBT that leaves no
 * decay region fails the build rather than the run.
 *
 * Two properties of the SBT make this non-trivial, and both are handled here:
 *
 *  - **The X envelope is not linear in Z.** Side containers are split at the
 *    column front-flange edge (SBT::zSplitOffset()) and the upstream piece
 *    has a *flat* outer face frozen at the sub-frustum's entrance
 *    half-width, so it does not clash with the column. The inner surface is
 *    therefore a sawtooth, dipping inward by up to xGrowth()*zSplitOffset()
 *    relative to the frustum. A single linear GeoTrap cannot follow it.
 *
 *  - **The innermost material in Y is steel, not scintillator.** The top and
 *    bottom longitudinal beams straddle the scintillator plane and their
 *    inner flange hangs *below* it, into the decay region. It sits deeper
 *    than the cells by (hbeam_height - flange_thickness) - container_thickness
 *    + sensor_clearance, and it — not the scintillator — bounds the helium.
 *
 * Consequently the helium is built as a stack of GeoTraps, two per
 * sub-frustum, each exactly filling the envelope over its Z span.
 */

/// One Z slab of the helium: a GeoTrap with rectangular faces at z_lo/z_hi.
struct HeliumPiece {
    double z_lo_mm = 0.0;
    double z_hi_mm = 0.0;
    double dx_lo_mm = 0.0;  ///< half-width in X at z_lo
    double dx_hi_mm = 0.0;  ///< half-width in X at z_hi
    double dy_lo_mm = 0.0;  ///< half-height in Y at z_lo
    double dy_hi_mm = 0.0;  ///< half-height in Y at z_hi
};

namespace detail {

// Index of the sub-frustum containing z (clamped at the exit face).
//
// The cast truncates towards zero rather than flooring, which lands on the
// same index for every f: the two agree for f >= 0, and a negative f clamps
// to 0 whichever way it was rounded. Truncating also keeps this header clear
// of C++23's constexpr <cmath>, which not every standard library ships yet.
constexpr int subFrustumAt(double z_mm, const SBTParams& params = kSBT) {
    const double f = (z_mm - params.zEntrance) / subLength(params);
    return std::clamp(static_cast<int>(f), 0, params.nSubFrustum - 1);
}

// The half-width the SIDE containers track at z. Inside the flat piece of a
// sub-frustum they are frozen at that sub-frustum's entrance half-width (see
// SBTSensorBuilder::placeSideContainer), which is what makes the X envelope a
// sawtooth rather than a straight line.
constexpr double sideTrackedXHalf(double z_mm, const SBTParams& params = kSBT) {
    const int s = subFrustumAt(z_mm, params);
    const double zLo = zSubLo(s, params);
    if (z_mm <= zLo + zSplitOffset(params)) {
        return xHalfAt(zLo, params);
    }
    return xHalfAt(z_mm, params);
}

}  // namespace detail

/**
 * @brief |X| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in X.
 * Currently only the side scintillator containers do; the columns and corner
 * beams sit a further half-flange-width outboard.
 */
constexpr double innerFreeHalfX(double z_mm, const SBTParams& params = kSBT) {
    // Side scintillator containers. (Columns and corner beams are outboard of
    // these by construction: their inner face is at x_half - flange_width/2,
    // a full container_thickness further out.)
    return sideSensorInnerX(detail::sideTrackedXHalf(z_mm, params), params);
}

/**
 * @brief |Y| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in Y: the
 * top/bottom scintillator containers *and* the inner flange of the top/bottom
 * longitudinal beams, which is the binding one.
 */
constexpr double innerFreeHalfY(double z_mm, const SBTParams& params = kSBT) {
    const double yHalf = yHalfAt(z_mm, params);
    // Top/bottom scintillator containers ...
    const double sensor = topBottomSensorInnerY(yHalf, params);
    // ... and the longitudinal beams' inner flange, which hangs below them.
    const double beam = longBeamInnerY(yHalf, params);
    // (Cross-beams sit a full beam-height above y_half and never reach in.)
    //
    // KNOWN LIMITATION (conservative): the longitudinal beams do not run the
    // full sub-frustum length — they stand off by ~0.5*flange_width from each
    // boundary (SBTStructureBuilder). In those ~142 mm end bands the true Y
    // bound is the shallower scintillator, but we cap at the deeper beam line
    // regardless, so the helium is up to ~13.5 mm short of the material there.
    // This never overlaps (it only makes the helium smaller), but it does leave
    // the helium slightly inside the "no unphysical margin" ideal over ~2.8 m of
    // length — worth ~0.18 m^3, i.e. 0.04% of the fiducial volume. Reclaiming it
    // means subdividing the Y envelope at the beam ends, which has to account
    // for the flange box's z-projection overshooting its nominal end; deferred
    // to a dedicated change rather than risking that interaction here.
    return std::min(sensor, beam);
}

namespace detail {

/**
 * @brief Z of envelope knot @p i, for 0 <= i <= 2*nSubFrustum.
 *
 * innerFreeHalfX/Y are piecewise linear and change slope exactly here — at
 * each sub-frustum boundary, and at the end of each sub-frustum's flat piece
 * — so a linear shape that respects the envelope at these Z values respects
 * it everywhere. The helium slab boundaries are drawn from them.
 *
 * Indexed rather than tabulated because the table's *type* would carry
 * nSubFrustum, and nothing outside this header ever read the table.
 */
constexpr double knotAt(int i, const SBTParams& params = kSBT) {
    if (i >= 2 * params.nSubFrustum) {
        return zExit(params);
    }
    const int s = i / 2;
    return (i % 2 == 0) ? zSubLo(s, params) : zSubLo(s, params) + zSplitOffset(params);
}

// The X envelope STEPS at each zSplitOffset knot: the flat piece's inner face
// sits at x_half(zLo)-..., and the tracking piece that follows begins at
// x_half(zSplit)-..., xGrowth*zSplitOffset further out. Sampling the outboard
// branch at the knot would put the helium's leading edge in the same plane as
// the flat piece's inner face — no overlap in the strict sense, but a
// coincident surface, which Geant4's navigator will not thank us for, and
// which silently eats the clearance besides.
//
// So evaluate the envelope as a *closed* set: at a knot, take the smaller of
// the two one-sided limits. The helium is then continuous, strictly inscribed,
// and keeps its full clearance everywhere.
constexpr double envelopeAtKnot(double z_mm, bool isX, const SBTParams& params = kSBT) {
    constexpr double kEps = 1e-6;
    const double lo = std::max(z_mm - kEps, params.zEntrance);
    const double hi = std::min(z_mm + kEps, zExit(params));
    return isX ? std::min(innerFreeHalfX(lo, params), innerFreeHalfX(hi, params))
               : std::min(innerFreeHalfY(lo, params), innerFreeHalfY(hi, params));
}

}  // namespace detail

/**
 * @brief Helium slab @p i, for 0 <= i < 2*nSubFrustum.
 *
 * Each slab is inset from innerFreeHalfX/Y by params.heliumClearance and spans
 * one envelope segment (two per sub-frustum).
 *
 * This is the single definition of the helium. kHeliumPieces is this function
 * tabulated for kSBT; leavesDecayRegion() walks it without tabulating, which
 * is what lets it answer for an nSubFrustum no array could hold.
 */
constexpr HeliumPiece heliumPieceAt(int i, const SBTParams& params = kSBT) {
    const double zLo = detail::knotAt(i, params);
    const double zHi = detail::knotAt(i + 1, params);
    return HeliumPiece{
        .z_lo_mm = zLo,
        .z_hi_mm = zHi,
        .dx_lo_mm = detail::envelopeAtKnot(zLo, /*isX=*/true, params) - params.heliumClearance,
        .dx_hi_mm = detail::envelopeAtKnot(zHi, /*isX=*/true, params) - params.heliumClearance,
        .dy_lo_mm = detail::envelopeAtKnot(zLo, /*isX=*/false, params) - params.heliumClearance,
        .dy_hi_mm = detail::envelopeAtKnot(zHi, /*isX=*/false, params) - params.heliumClearance,
    };
}

/**
 * @brief Does @p params leave a decay region for the helium to fill?
 *
 * False if the beams, containers or clearances have swallowed the frustum.
 * Three ways that happens, all covered here:
 *
 *  - a slab has non-positive half-width (containers wider than the frustum);
 *  - two knots cross, i.e. a sub-frustum is shorter than the sensors' flat
 *    piece — knotAt(2s+1) = zSubLo(s) + zSplitOffset() then overshoots
 *    knotAt(2s+2) = zSubLo(s+1), so the slab between them is inverted. This
 *    subsumes the zSplitOffset() < subLength() assertion in SBTConstants.h,
 *    which is kept only because its message is more specific and because it
 *    also constrains SBTSensorBuilder;
 *  - a negative clearance, which puts the helium inside the SBT by
 *    construction and cannot be caught by a degeneracy test — it makes the
 *    slabs *larger*.
 *
 * Formerly a runtime throw from heliumPieces(cfg). Used by the static_assert
 * below for the shipped parameters, and by test_decayvolume for the
 * configurations that must not build.
 */
constexpr bool leavesDecayRegion(const SBTParams& params = kSBT) {
    if (params.nSubFrustum <= 0 || params.heliumClearance < 0.0) {
        return false;
    }
    for (int i = 0; i < 2 * params.nSubFrustum; ++i) {
        const HeliumPiece piece = heliumPieceAt(i, params);
        if (piece.z_hi_mm <= piece.z_lo_mm) {
            return false;
        }
        if (piece.dx_lo_mm <= 0.0 || piece.dx_hi_mm <= 0.0 || piece.dy_lo_mm <= 0.0 ||
            piece.dy_hi_mm <= 0.0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief The helium slabs of the shipped SBT.
 *
 * Exactly sized, and the only stack the geometry ever builds: buildHelium()
 * defaults to it so that nothing on the production path re-derives the helium
 * at run time (see the note on buildHelium).
 */
inline constexpr auto kHeliumPieces = [] {
    std::array<HeliumPiece, 2 * kSBT.nSubFrustum> pieces{};
    for (int i = 0; i < 2 * kSBT.nSubFrustum; ++i) {
        pieces[static_cast<std::size_t>(i)] = heliumPieceAt(i, kSBT);
    }
    return pieces;
}();

// Formerly a runtime throw: an SBT whose containers, beams or clearances
// swallow the decay region fails the build.
static_assert(leavesDecayRegion(kSBT),
              "the configured SBT leaves no decay region — check containerThickness, "
              "hbeamHeight, nSubFrustum and heliumClearance");

// The PR's headline claim as a build failure rather than a promise: the
// shipped slabs are bit-for-bit what they were before the SBT parameters were
// grouped into a struct. Changing the SBT deliberately means recomputing this
// deliberately — print the new value by temporarily comparing against 0.
constexpr std::uint64_t heliumFingerprint() {
    std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a over the raw bit patterns
    for (const HeliumPiece& piece : kHeliumPieces) {
        for (const double d : {piece.z_lo_mm, piece.z_hi_mm, piece.dx_lo_mm, piece.dx_hi_mm,
                               piece.dy_lo_mm, piece.dy_hi_mm}) {
            h = (h ^ std::bit_cast<std::uint64_t>(d)) * 0x100000001b3ULL;
        }
    }
    return h;
}
static_assert(heliumFingerprint() == 0xfffbdca57c0cfc67ULL, "the helium slabs moved");

// ── Arbitrary parameters, for the tests ─────────────────────────────────
//
//  std::array's extent is part of its type, and test_decayvolume sweeps
//  nSubFrustum between 5 and 20, so a swept stack needs one type that covers
//  them all. This is a testing affordance, not a geometry parameter: the
//  shipped stack is kHeliumPieces above and is exactly sized.

inline constexpr int kMaxSubFrustum = 64;
static_assert(kSBT.nSubFrustum <= kMaxSubFrustum);

struct HeliumSlabs {
    std::array<HeliumPiece, 2u * kMaxSubFrustum> pieces{};
    std::size_t count = 0;
    /// A view of the filled prefix. Borrows from *this — keep it alive.
    [[nodiscard]] constexpr std::span<const HeliumPiece> view() const {
        return {pieces.data(), count};
    }
};

/// The helium stack for arbitrary @p params. A count of 0 means the request
/// was out of range (nSubFrustum outside 1..kMaxSubFrustum); ask
/// leavesDecayRegion() whether a configuration is buildable at all.
constexpr HeliumSlabs heliumSlabs(const SBTParams& params = kSBT) {
    HeliumSlabs slabs;
    if (params.nSubFrustum <= 0 || params.nSubFrustum > kMaxSubFrustum) {
        return slabs;
    }
    slabs.count = 2u * static_cast<std::size_t>(params.nSubFrustum);
    for (std::size_t i = 0; i < slabs.count; ++i) {
        slabs.pieces[i] = heliumPieceAt(static_cast<int>(i), params);
    }
    return slabs;
}

/**
 * @brief Place helium slabs into @p container.
 *
 * Call it *after* SBTStructureBuilder / SBTSensorBuilder — the helium is a
 * consequence of them.
 *
 * Takes the slabs, not the parameters, and that is deliberate: defaulting to
 * the constexpr kHeliumPieces makes "the shipped geometry is byte-identical" a
 * property of the signature rather than of the optimiser. Deriving the slabs
 * here from an SBTParams would move the arithmetic to run time, where
 * -ffp-contract=fast (GCC's C++ default) may contract `a + (z - b) * c` into
 * an FMA; constant evaluation never contracts. It would look like a tidy-up
 * and would shift the helium by a ULP.
 */
void buildHelium(GeoPhysVol* container, const GeoMaterial* helium,
                 std::span<const HeliumPiece> pieces = kHeliumPieces);

}  // namespace SHiPGeometry::SBT
