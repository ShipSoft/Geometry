// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

class GeoVPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;
class SHiPUBTManager;

/**
 * @brief Factory for the UpstreamTagger (upstream veto tagger) geometry.
 *
 * Builds the segmented "all-tile" upstream background tagger: a single plane
 * of polystyrene scintillator tiles, coplanar in Z, tiling the transverse
 * plane with two granularities:
 *
 *   - Fine   tiles: 20 × 20 mm² face, 20 mm pitch.
 *   - Coarse tiles: 40 × 40 mm² face, 40 mm pitch.
 *
 * The tiles are grouped into seven abutting regions (origin at the plane
 * centre; X horizontal, Y vertical, Z along the beam):
 *
 *      Y
 *      ^
 *   +3200 +------+------------------------------------+------+
 *         | EXT  |        COARSE band (top)           | EXT  |  y=[+200,+3200]
 *   +200  |      +--------+------------------+--------+      |
 *         | EXT  |  FINE  |  COARSE central  |  FINE  | EXT  |  y=[-200,+200]
 *   -200  |      +--------+------------------+--------+      |
 *         | EXT  |        COARSE band (bottom)        | EXT  |  y=[-3200,-200]
 *   -3200 +------+------------------------------------+------+
 *        -2200  -1000   -600              +600    +1000   +2200  -> X
 *
 * Tile counts: fine blocks 2 × 400, coarse central 300, coarse bands
 * 2 × 3750, extensions 2 × 19200  → 47 000 tiles, covering the full
 * 4.4 × 6.4 m container cross-section. Fine tiles are 5 mm
 * thick, coarse tiles 10 mm; both are centred on the same Z plane.
 *
 * The whole assembly is returned as a single GeoFullPhysVol container (air),
 * so the tagger keeps a sensitive tree-top that can be registered with the
 * SHiPUBTManager. Individual tiles are GeoPhysVol with hierarchical
 * "/SHiP/upstream_tagger/..." names, mirroring the calorimeter bar layers;
 * sensitive-detector assignment is performed downstream by name pattern.
 *
 * The container envelope (half 2200 × 3200 × 80 mm, centre Z = 32 720 mm)
 * is unchanged from the previous monolithic slab so the placement in
 * SHiPGeometry and the subsystem-envelope consistency checks still hold.
 */
class UpstreamTaggerFactory {
   public:
    explicit UpstreamTaggerFactory(SHiPMaterials& materials);
    ~UpstreamTaggerFactory() = default;

    /**
     * @brief Build the UpstreamTagger geometry.
     * @param manager Optional manager to register the container tree-top; may be null.
     * @return Pointer to the GeoFullPhysVol tile-plane container.
     */
    [[nodiscard]] GeoVPhysVol* build(SHiPUBTManager* manager = nullptr);

    // ── Container envelope (mm) ─────────────────────────────────────────
    // The GDML statbox (440 × 640 × 16 cm). This is wider than the EDMS
    // envelope (0.75 × 1.60 m half-sizes); Layout.h records the deviation.
    static constexpr auto s_halfX = 2200.0 * units::mm;
    static constexpr auto s_halfY = 3200.0 * units::mm;
    static constexpr auto s_halfZ = 80.0 * units::mm;

    // ── Tile geometry (mm) ──────────────────────────────────────────────
    static constexpr auto s_fineThickness = 5.0 * units::mm;  ///< full Z thickness of fine tiles
    static constexpr auto s_coarseThickness =
        10.0 * units::mm;  ///< full Z thickness of coarse tiles
    static constexpr auto s_fineFace =
        20.0 * units::mm;  ///< fine tile full transverse size = pitch
    static constexpr auto s_coarseFace =
        40.0 * units::mm;  ///< coarse tile full transverse size = pitch

   private:
    SHiPMaterials& m_materials;
};

}  // namespace SHiPGeometry
