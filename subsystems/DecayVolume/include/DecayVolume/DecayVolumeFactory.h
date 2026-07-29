// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/SubsystemDescriptor.h"
#include "DecayVolume/SBTConfig.h"

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
 * The helium is not an independent volume: it is derived from where the SBT
 * material actually is (see SBTEnvelope.h), so that it can neither overlap the
 * structure and sensors nor leave an unphysical margin behind.
 *
 * Z: 32.92 to 83.32 m -> centre 58.12 m; placement handled by SHiPGeometry.
 */
class DecayVolumeFactory {
   public:
    explicit DecayVolumeFactory(SHiPMaterials& materials, std::string configPath = "sbt.toml");
    ~DecayVolumeFactory() = default;

    /// This subsystem's self-description (name, node, id, placement).
    static SubsystemDescriptor descriptor() {
        return {"DecayVolume", "/SHiP/decay_volume", 4, 0.0, 0.0, 58120.0, false};
    }
    /// Build the DecayVolume geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

    /// The config the last build() actually used.
    ///
    /// Tests must reason about *this*, not a default-constructed SBTConfig:
    /// the geometry comes from sbt.toml, and a test that checks a clearance
    /// against the C++ default is validating a config it did not build.
    [[nodiscard]] const SBTConfig& config() const { return m_config; }

   private:
    SHiPMaterials& m_materials;
    std::string m_configPath;
    SBTConfig m_config;

    // Air container enclosing the SBT structure + sensors and helium (mm).
    static constexpr double s_halfX = 2200.0;
    static constexpr double s_halfY = 3300.0;
    static constexpr double s_halfZ = 25200.0;
};

}  // namespace SHiPGeometry
