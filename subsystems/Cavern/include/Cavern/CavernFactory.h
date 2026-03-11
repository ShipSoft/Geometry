// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

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
    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* m_world{nullptr};
    GeoPhysVol* m_cavern{nullptr};

    // World volume dimensions (half-sizes in mm)
    static constexpr double s_worldHalfX = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfY = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfZ = 200.0 * GeoModelKernelUnits::m;

    // Rock block dimensions (half-sizes in mm)
    static constexpr double s_rockHalfX = 20.0 * GeoModelKernelUnits::m;
    static constexpr double s_rockHalfY = 20.0 * GeoModelKernelUnits::m;
    static constexpr double s_rockHalfZ = 140.0 * GeoModelKernelUnits::m;

    // Muon shield cavern (half-sizes and position in mm)
    static constexpr double s_muonCavernHalfX = 3.55 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernHalfY = 3.0 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernHalfZ = 41.0 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernPosX = 0.0;
    static constexpr double s_muonCavernPosY = 1.7 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernPosZ = -58.336 * GeoModelKernelUnits::m;

    // Experiment cavern (half-sizes and position in mm)
    static constexpr double s_expCavernHalfX = 12.0 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernHalfY = 11.65 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernHalfZ = 60.0 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernPosX = 0.0;
    static constexpr double s_expCavernPosY = 3.35 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernPosZ = 42.664 * GeoModelKernelUnits::m;

    // Stair step (half-sizes and position in mm)
    static constexpr double s_stairHalfX = 3.55 * GeoModelKernelUnits::m;
    static constexpr double s_stairHalfY = 4.325 * GeoModelKernelUnits::m;
    static constexpr double s_stairHalfZ = 8.0 * GeoModelKernelUnits::m;
    static constexpr double s_stairPosX = 0.0;
    static constexpr double s_stairPosY = -5.625 * GeoModelKernelUnits::m;
    static constexpr double s_stairPosZ = -9.336 * GeoModelKernelUnits::m;

    // Yoke pit (half-sizes and position in mm)
    static constexpr double s_yokePitHalfX = 6.0 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitHalfY = 5.0 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitHalfZ = 6.0 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitPosX = 0.0;
    static constexpr double s_yokePitPosY = -6.65 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitPosZ = 26.664 * GeoModelKernelUnits::m;

    // Target pit (half-sizes and position in mm)
    static constexpr double s_targetPitHalfX = 2.0 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitHalfY = 0.5 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitHalfZ = 2.0 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitPosX = 0.0;
    static constexpr double s_targetPitPosY = -2.2 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitPosZ = -103.336 * GeoModelKernelUnits::m;

    // Cavern position in world
    static constexpr double s_cavernPosZ = -3.336 * GeoModelKernelUnits::m;
};

}  // namespace SHiPGeometry
