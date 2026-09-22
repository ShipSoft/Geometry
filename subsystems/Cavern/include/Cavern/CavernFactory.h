// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Cavern (world volume and rock) geometry
 */
class CavernFactory {
   public:
    explicit CavernFactory(SHiPMaterials& materials);
    ~CavernFactory() = default;

    /**
     * @brief Build the Cavern geometry
     * @return Pointer to the world physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* m_world{nullptr};
    GeoPhysVol* m_cavern{nullptr};

    // World volume dimensions (half-sizes)
    static constexpr auto s_worldHalfX = 200.0 * units::m;
    static constexpr auto s_worldHalfY = 200.0 * units::m;
    static constexpr auto s_worldHalfZ = 200.0 * units::m;

    // Rock block dimensions (half-sizes)
    static constexpr auto s_rockHalfX = 20.0 * units::m;
    static constexpr auto s_rockHalfY = 20.0 * units::m;
    static constexpr auto s_rockHalfZ = 140.0 * units::m;

    // Muon shield cavern (half-sizes and position)
    static constexpr auto s_muonCavernHalfX = 3.55 * units::m;
    static constexpr auto s_muonCavernHalfY = 3.0 * units::m;
    static constexpr auto s_muonCavernHalfZ = 41.0 * units::m;
    static constexpr auto s_muonCavernPosX = 0.0 * units::m;
    static constexpr auto s_muonCavernPosY = 1.7 * units::m;
    static constexpr auto s_muonCavernPosZ = -58.336 * units::m;

    // Experiment cavern (half-sizes and position)
    static constexpr auto s_expCavernHalfX = 12.0 * units::m;
    static constexpr auto s_expCavernHalfY = 11.65 * units::m;
    static constexpr auto s_expCavernHalfZ = 60.0 * units::m;
    static constexpr auto s_expCavernPosX = 0.0 * units::m;
    static constexpr auto s_expCavernPosY = 3.35 * units::m;
    static constexpr auto s_expCavernPosZ = 42.664 * units::m;

    // Stair step (half-sizes and position)
    static constexpr auto s_stairHalfX = 3.55 * units::m;
    static constexpr auto s_stairHalfY = 4.325 * units::m;
    static constexpr auto s_stairHalfZ = 8.0 * units::m;
    static constexpr auto s_stairPosX = 0.0 * units::m;
    static constexpr auto s_stairPosY = -5.625 * units::m;
    static constexpr auto s_stairPosZ = -9.336 * units::m;

    // Yoke pit (half-sizes and position)
    static constexpr auto s_yokePitHalfX = 6.0 * units::m;
    static constexpr auto s_yokePitHalfY = 5.0 * units::m;
    static constexpr auto s_yokePitHalfZ = 6.0 * units::m;
    static constexpr auto s_yokePitPosX = 0.0 * units::m;
    static constexpr auto s_yokePitPosY = -6.65 * units::m;
    static constexpr auto s_yokePitPosZ = 26.664 * units::m;

    // Target pit (half-sizes and position)
    static constexpr auto s_targetPitHalfX = 2.0 * units::m;
    static constexpr auto s_targetPitHalfY = 0.5 * units::m;
    static constexpr auto s_targetPitHalfZ = 2.0 * units::m;
    static constexpr auto s_targetPitPosX = 0.0 * units::m;
    static constexpr auto s_targetPitPosY = -2.2 * units::m;
    static constexpr auto s_targetPitPosZ = -103.336 * units::m;

    // Cavern position in world
    static constexpr auto s_cavernPosZ = -3.336 * units::m;
};

}  // namespace SHiPGeometry
