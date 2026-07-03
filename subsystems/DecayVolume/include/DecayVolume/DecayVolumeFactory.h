// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the DecayVolume subsystem (decay region + SBT).
 *
 * Builds an air container holding the Surround Background Tagger — a steel
 * H-beam supporting structure and LAB scintillator sensor cells forming a
 * 50 m rectangular frustum — wrapped around a central helium decay volume.
 * The SBT geometry is driven by sbt.toml (parsed into an SBTConfig).
 *
 * The helium frustum is sized strictly inside the innermost sensor faces, so
 * it does not overlap the structure or the sensors.
 *
 * Z: 32.92 to 83.32 m -> centre 58.12 m; placement handled by SHiPGeometry.
 */
class DecayVolumeFactory {
   public:
    explicit DecayVolumeFactory(SHiPMaterials& materials, std::string configPath = "sbt.toml");
    ~DecayVolumeFactory() = default;

    /// Build the DecayVolume geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;
    std::string m_configPath;

    // Air container enclosing the SBT structure + sensors and helium (mm).
    static constexpr double s_halfX = 2200.0;
    static constexpr double s_halfY = 3300.0;
    static constexpr double s_halfZ = 25200.0;
};

}  // namespace SHiPGeometry
