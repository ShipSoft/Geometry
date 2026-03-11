// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoVPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;
class SHiPUBTManager;

/**
 * @brief Factory for the Upstream Background Tagger (UBT) geometry.
 *
 * Builds a segmented 2×3 m² tracker plane of drift tubes and scintillator tiles.
 *
 * ## Detector layout
 *
 * ```
 *   Y
 *   +1500 ┌──────────────────────────────────────────┐
 *         │  Top_Left tubes  +  Top_Right tubes       │  y = [+200, +1500]
 *   +200  ├───────────┬──────────────────┬────────────┤
 *         │ PS tiles  │  Central tubes   │  PS tiles  │  y = [-200, +200]
 *         │ (left)    │  x=[-600,+600]   │  (right)   │
 *   -200  ├───────────┴──────────────────┴────────────┤
 *         │  Bottom_Left tubes + Bottom_Right tubes   │  y = [-1500, -200]
 *   -1500 └──────────────────────────────────────────┘
 *         -1000 -600                  +600 +1000     → X (mm)
 * ```
 *
 * ## Sensitive volumes
 * All tube gas volumes (GeoFullPhysVol, LV name contains "TubeGas_LV") and
 * tile volumes (GeoFullPhysVol, LV name "UBT_Tile_LV") are registered with
 * the supplied SHiPUBTManager for downstream Geant4 SD registration.
 *
 * ## Envelope dimensions (half-extents, mm)
 *   halfX = 1000,  halfY = 1500,  halfZ = 8
 *
 * ## Position in world
 *   z = 32720 mm (centre of the 32520–32920 mm z range)
 */
class UpstreamTaggerFactory {
   public:
    explicit UpstreamTaggerFactory(SHiPMaterials& materials);
    ~UpstreamTaggerFactory() = default;

    /**
     * @brief Build the full segmented UBT plane.
     * @param manager Optional manager to register sensitive volumes; may be null.
     * @return Pointer to the envelope GeoPhysVol.
     */
    GeoVPhysVol* build(SHiPUBTManager* manager = nullptr);

    // Envelope half-dimensions (mm) — used by the test to check against CSV limits
    // Envelope half-dimensions match CSV: half_width=750mm, half_height=1600mm
    static constexpr double s_halfX =  750.0;
    static constexpr double s_halfY = 1600.0;
    static constexpr double s_halfZ =    8.0;

   private:
    SHiPMaterials& m_materials;

    static constexpr double s_tubeROuter_mm  =  2.5;
    static constexpr double s_tubeWall_mm    =  0.015;
    static constexpr double s_tubeHalfLen_mm = 600.0;
    static constexpr double s_tileSide_mm    =  10.0;
};

}  // namespace SHiPGeometry
