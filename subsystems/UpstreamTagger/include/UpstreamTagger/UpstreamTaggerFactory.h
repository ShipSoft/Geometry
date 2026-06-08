// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

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
 *   +1500 +------+------------------------------------+------+
 *         | EXT  |        COARSE band (top)           | EXT  |  y=[+200,+1500]
 *   +200  |      +--------+------------------+--------+      |
 *         | EXT  |  FINE  |  COARSE central  |  FINE  | EXT  |  y=[-200,+200]
 *   -200  |      +--------+------------------+--------+      |
 *         | EXT  |        COARSE band (bottom)        | EXT  |  y=[-1500,-200]
 *   -1500 +------+------------------------------------+------+
 *        -1800  -1000   -600              +600    +1000   +1800  -> X
 *
 * Tile counts: fine blocks 2 × 400, coarse central 300, coarse bands
 * 2 × 1600, extensions 2 × 6000  → 16 300 tiles. All 10 mm thick.
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
    GeoVPhysVol* build(SHiPUBTManager* manager = nullptr);

   private:
    SHiPMaterials& m_materials;

    // ── Container envelope (mm) ─────────────────────────────────────────
    // Kept identical to the GDML statbox (440 × 640 × 16 cm) used by the
    // previous monolithic slab so placement and the envelope-fit test
    // (halfX ≤ 2200, halfY ≤ 3200, halfZ ≤ 200) are unchanged.
    static constexpr double s_halfX = 2200.0;
    static constexpr double s_halfY = 3200.0;
    static constexpr double s_halfZ = 80.0;

    // ── Tile geometry (mm) ──────────────────────────────────────────────
    static constexpr double s_tileThickness = 10.0;  ///< full Z thickness of every tile
    static constexpr double s_fineFace = 20.0;       ///< fine tile full transverse size = pitch
    static constexpr double s_coarseFace = 40.0;     ///< coarse tile full transverse size = pitch
};

}  // namespace SHiPGeometry
