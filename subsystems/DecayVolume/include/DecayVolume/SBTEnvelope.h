// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "DecayVolume/SBTConstants.h"

#include <algorithm>
#include <array>
#include <cmath>

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
constexpr int subFrustumAt(double z_mm) {
    const double f = (z_mm - kZEntrance) / subLength();
    return std::clamp(static_cast<int>(std::floor(f)), 0, kNSubFrustum - 1);
}

// The half-width the SIDE containers track at z. Inside the flat piece of a
// sub-frustum they are frozen at that sub-frustum's entrance half-width (see
// SBTSensorBuilder::placeSideContainer), which is what makes the X envelope a
// sawtooth rather than a straight line.
constexpr double sideTrackedXHalf(double z_mm) {
    const int s = subFrustumAt(z_mm);
    const double zLo = zSubLo(s);
    if (z_mm <= zLo + zSplitOffset())
        return xHalfAt(zLo);
    return xHalfAt(z_mm);
}

}  // namespace detail

/**
 * @brief |X| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in X.
 * Currently only the side scintillator containers do; the columns and corner
 * beams sit a further half-flange-width outboard.
 */
constexpr double innerFreeHalfX(double z_mm) {
    // Side scintillator containers. (Columns and corner beams are outboard of
    // these by construction: their inner face is at x_half - flange_width/2,
    // a full container_thickness further out.)
    return sideSensorInnerX(detail::sideTrackedXHalf(z_mm));
}

/**
 * @brief |Y| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in Y: the
 * top/bottom scintillator containers *and* the inner flange of the top/bottom
 * longitudinal beams, which is the binding one.
 */
constexpr double innerFreeHalfY(double z_mm) {
    const double yHalf = yHalfAt(z_mm);
    // Top/bottom scintillator containers ...
    const double sensor = topBottomSensorInnerY(yHalf);
    // ... and the longitudinal beams' inner flange, which hangs below them.
    const double beam = longBeamInnerY(yHalf);
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

/**
 * @brief Z values at which innerFreeHalfX/Y change slope.
 *
 * innerFreeHalfX/Y are piecewise linear with knots exactly here, so a linear
 * shape that respects the envelope at these Z values respects it everywhere.
 * The helium slab boundaries are drawn from this list. The knot layout
 * assumes each sub-frustum is long enough to contain its flat piece —
 * guaranteed by the zSplitOffset() < subLength() static_assert in
 * SBTConstants.h.
 */
inline constexpr auto kEnvelopeKnots = [] {
    std::array<double, 2 * kNSubFrustum + 1> knots{};
    for (int s = 0; s < kNSubFrustum; ++s) {
        knots[2 * s] = zSubLo(s);                       // sub-frustum boundary
        knots[2 * s + 1] = zSubLo(s) + zSplitOffset();  // end of the flat piece
    }
    knots[2 * kNSubFrustum] = zExit();
    return knots;
}();

namespace detail {

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
constexpr double envelopeAtKnot(double z_mm, bool isX) {
    constexpr double kEps = 1e-6;
    const double lo = std::max(z_mm - kEps, kZEntrance);
    const double hi = std::min(z_mm + kEps, zExit());
    return isX ? std::min(innerFreeHalfX(lo), innerFreeHalfX(hi))
               : std::min(innerFreeHalfY(lo), innerFreeHalfY(hi));
}

}  // namespace detail

/**
 * @brief The helium slabs filling the SBT interior.
 *
 * Each slab is inset from innerFreeHalfX/Y by kHeliumClearance (two per
 * sub-frustum, one per envelope segment).
 */
inline constexpr auto kHeliumPieces = [] {
    std::array<HeliumPiece, 2 * kNSubFrustum> pieces{};
    for (std::size_t i = 0; i + 1 < kEnvelopeKnots.size(); ++i) {
        const double zLo = kEnvelopeKnots[i];
        const double zHi = kEnvelopeKnots[i + 1];

        HeliumPiece& p = pieces[i];
        p.z_lo_mm = zLo;
        p.z_hi_mm = zHi;
        p.dx_lo_mm = detail::envelopeAtKnot(zLo, /*isX=*/true) - kHeliumClearance;
        p.dx_hi_mm = detail::envelopeAtKnot(zHi, /*isX=*/true) - kHeliumClearance;
        p.dy_lo_mm = detail::envelopeAtKnot(zLo, /*isX=*/false) - kHeliumClearance;
        p.dy_hi_mm = detail::envelopeAtKnot(zHi, /*isX=*/false) - kHeliumClearance;
    }
    return pieces;
}();

// Formerly a runtime throw: an SBT whose containers, beams or clearances
// swallow the decay region fails the build.
static_assert(
    [] {
        for (const HeliumPiece& p : kHeliumPieces) {
            if (p.z_hi_mm <= p.z_lo_mm)
                return false;
            if (p.dx_lo_mm <= 0.0 || p.dx_hi_mm <= 0.0 || p.dy_lo_mm <= 0.0 || p.dy_hi_mm <= 0.0)
                return false;
        }
        return true;
    }(),
    "the configured SBT leaves no decay region");

/**
 * @brief Place the helium slabs (kHeliumPieces) into @p container.
 *
 * Call it *after* SBTStructureBuilder / SBTSensorBuilder — the helium is a
 * consequence of them.
 */
void buildHelium(GeoPhysVol* container, const GeoMaterial* helium);

}  // namespace SHiPGeometry::SBT
