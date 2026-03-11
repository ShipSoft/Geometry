// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the DecayVolume (decay vessel and veto system) geometry
 *
 * Two-volume nested structure: aluminium vacuum vessel with helium atmosphere.
 * Outer Aluminium shell: full envelope dimensions from CSV.
 * Inner PressurisedHe90 volume: envelope minus wall thicknesses.
 * Z: 32.92 to 83.32 m → centre: 58.12 m, half-length: 25.20 m
 */
class DecayVolumeFactory {
   public:
    explicit DecayVolumeFactory(SHiPMaterials& materials);
    ~DecayVolumeFactory() = default;

    /**
     * @brief Build the DecayVolume geometry
     * @return Pointer to the outer Aluminium vessel physical volume
     */
    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // Wall thickness of the aluminium vessel (mm)
    static constexpr double s_wallThickness = 20.0;

    // Outer envelope dimensions from CSV (mm)
    static constexpr double s_halfX = 1450.0;   // 1.45 m (average)
    static constexpr double s_halfY = 2375.0;   // 2.375 m (average)
    static constexpr double s_halfZ = 25200.0;  // 25.20 m

    // Inner helium volume (envelope minus wall thickness per axis)
    static constexpr double s_innerHalfX = s_halfX - s_wallThickness;
    static constexpr double s_innerHalfY = s_halfY - s_wallThickness;
    static constexpr double s_innerHalfZ = s_halfZ - s_wallThickness;
};

}  // namespace SHiPGeometry
