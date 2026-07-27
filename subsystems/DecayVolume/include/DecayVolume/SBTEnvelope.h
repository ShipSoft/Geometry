// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <vector>

class GeoMaterial;
class GeoPhysVol;

namespace SHiPGeometry {

struct SBTConfig;

/**
 * @brief The innermost free region of the SBT, and the helium that fills it.
 *
 * The helium decay region must fill the space inside the SBT exactly: it may
 * not protrude into any steel or scintillator, and it may not leave an
 * arbitrary safety margin behind either. That makes its size a *consequence*
 * of where the SBT material is, never an independent parameter.
 *
 * This header therefore derives the helium purely from the placement
 * primitives on SBTConfig — the same ones SBTStructureBuilder and
 * SBTSensorBuilder place their volumes with. Change a placement rule and the
 * helium follows automatically; there is no second copy of the arithmetic to
 * forget to update.
 *
 * Two properties of the SBT make this non-trivial, and both are handled here:
 *
 *  - **The X envelope is not linear in Z.** Side containers are split at the
 *    column front-flange edge (SBTConfig::zSplitOffset()) and the upstream
 *    piece has a *flat* outer face frozen at the sub-frustum's entrance
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

/**
 * @brief |X| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in X.
 * Currently only the side scintillator containers do; the columns and corner
 * beams sit a further half-flange-width outboard.
 */
double innerFreeHalfX(const SBTConfig& cfg, double z_mm);

/**
 * @brief |Y| of the innermost SBT material at @p z_mm (mm).
 *
 * Minimum over every volume class that can reach the decay region in Y: the
 * top/bottom scintillator containers *and* the inner flange of the top/bottom
 * longitudinal beams, which is the binding one.
 */
double innerFreeHalfY(const SBTConfig& cfg, double z_mm);

/**
 * @brief Z values at which innerFreeHalfX/Y change slope.
 *
 * innerFreeHalfX/Y are piecewise linear with knots exactly here, so a linear
 * shape that respects the envelope at these Z values respects it everywhere.
 * The helium slab boundaries are drawn from this list.
 */
std::vector<double> envelopeKnots(const SBTConfig& cfg);

/**
 * @brief The helium slabs filling the SBT interior.
 *
 * Each slab is inset from innerFreeHalfX/Y by cfg.helium_clearance_mm (0 by
 * default: the helium touches the SBT and leaves no margin).
 *
 * @throws std::runtime_error if any slab would have a non-positive half-width,
 *         i.e. the configured beams/containers have swallowed the decay region.
 */
std::vector<HeliumPiece> heliumPieces(const SBTConfig& cfg);

/**
 * @brief Place the helium slabs into @p container.
 *
 * Mirrors SBTStructureBuilder / SBTSensorBuilder: takes an SBTConfig and nothing
 * else, so the geometry can be built (and swept) without going through sbt.toml.
 * Call it *after* the two builders — the helium is a consequence of them.
 */
void buildHelium(GeoPhysVol* container, const GeoMaterial* helium, const SBTConfig& cfg);

}  // namespace SHiPGeometry
