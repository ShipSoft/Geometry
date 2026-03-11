// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoVPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;
class SHiPUBTManager;

/**
 * @brief Factory for the UpstreamTagger (upstream veto tagger) geometry
 *
 * Creates a scintillator slab as a GeoFullPhysVol (sensitive volume).
 * Based on GDML reference: Box 440×640×16 cm (full dimensions).
 * Z: 32.52 to 32.92 m → centre: 32.72 m, half-length: 0.20 m
 * Half-width: 2.20 m, half-height: 3.20 m
 */
class UpstreamTaggerFactory {
   public:
    explicit UpstreamTaggerFactory(SHiPMaterials& materials);
    ~UpstreamTaggerFactory() = default;

    /**
     * @brief Build the UpstreamTagger geometry.
     * @param manager Optional manager to register the sensitive slab; may be null.
     * @return Pointer to the GeoFullPhysVol scintillator slab.
     */
    GeoVPhysVol* build(SHiPUBTManager* manager = nullptr);

   private:
    SHiPMaterials& m_materials;

    // Dimensions from GDML: Box 440×640×16 cm → half: 220×320×8 cm (mm)
    static constexpr double s_halfX = 2200.0;
    static constexpr double s_halfY = 3200.0;
    static constexpr double s_halfZ = 80.0;
};

}  // namespace SHiPGeometry
