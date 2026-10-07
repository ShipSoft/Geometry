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
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* m_world{nullptr};
    GeoPhysVol* m_cavern{nullptr};

    // World volume dimensions (half-sizes in mm)
    static constexpr double s_worldHalfX = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfY = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfZ = 200.0 * GeoModelKernelUnits::m;

    // ── Layout (global frame: origin at the target front face, mm) ──────
    //
    // Integration layout SPSXXSHIP0002 version 2026-0.1 (EDMS 3287817 v1.1)
    // and the CAD model ST1967028_01. The TCC8/ECN3 step sits at the
    // mid-plane of the gap between muon-shield magnets M4 and S5; the beam
    // line is 1.70 m above the TCC8 floor and 3.36 m above the ECN3 floor.
    // The transverse offsets of the tunnel and the hall and the stair step
    // between them are carried over from the FairShip cavern (ShipCave).
    static constexpr double s_stepZ = 21.78 * GeoModelKernelUnits::m;  // TCC8/ECN3 step
    static constexpr double s_tcc8Length =
        170.0 * GeoModelKernelUnits::m;  // tunnel modelled upstream of the step
    static constexpr double s_stairLength =
        0.82 * GeoModelKernelUnits::m;  // stair step at the start of ECN3
    static constexpr double s_ecn3EndZ =
        120.48 * GeoModelKernelUnits::m;  // downstream end wall of ECN3
    static constexpr double s_tcc8FloorY = -1.70 * GeoModelKernelUnits::m;
    static constexpr double s_ecn3FloorY = -3.36 * GeoModelKernelUnits::m;
    static constexpr double s_magnetZ =
        89.72 * GeoModelKernelUnits::m;  // spectrometer magnet mid-plane
    static constexpr double s_targetEndZ =
        1.46 * GeoModelKernelUnits::m;  // downstream face of the target

    // Rock block (half-sizes in mm), centred on the modelled tunnel + hall
    static constexpr double s_rockHalfX = 20.0 * GeoModelKernelUnits::m;
    static constexpr double s_rockHalfY = 20.0 * GeoModelKernelUnits::m;
    static constexpr double s_rockHalfZ = 140.0 * GeoModelKernelUnits::m;
    static constexpr double s_cavernPosZ = (s_stepZ - s_tcc8Length + s_ecn3EndZ) / 2.0;

    // Cavity positions below are relative to the rock centre.

    // TCC8 tunnel (muon shield cavern): 9.99 m wide, 7.5 m high, ending at the step
    static constexpr double s_muonCavernHalfX = 4.995 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernHalfY = 3.75 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernHalfZ = s_tcc8Length / 2.0;
    static constexpr double s_muonCavernPosX = 1.435 * GeoModelKernelUnits::m;
    static constexpr double s_muonCavernPosY = s_tcc8FloorY + s_muonCavernHalfY;
    static constexpr double s_muonCavernPosZ = s_stepZ - s_muonCavernHalfZ - s_cavernPosZ;

    // Stair step at the start of ECN3 (floor 0.86 m below the TCC8 floor)
    static constexpr double s_stairHalfX = 7.995 * GeoModelKernelUnits::m;
    static constexpr double s_stairHalfY = 5.6 * GeoModelKernelUnits::m;
    static constexpr double s_stairHalfZ = s_stairLength / 2.0;
    static constexpr double s_stairPosX = 3.435 * GeoModelKernelUnits::m;
    static constexpr double s_stairPosY = 3.04 * GeoModelKernelUnits::m;
    static constexpr double s_stairPosZ = s_stepZ + s_stairHalfZ - s_cavernPosZ;

    // ECN3 hall (experiment cavern): 15.99 m wide, 12 m high, from the stair step to the end wall
    static constexpr double s_expCavernHalfX = 7.995 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernHalfY = 6.0 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernHalfZ = (s_ecn3EndZ - s_stepZ - s_stairLength) / 2.0;
    static constexpr double s_expCavernPosX = 3.435 * GeoModelKernelUnits::m;
    static constexpr double s_expCavernPosY = s_ecn3FloorY + s_expCavernHalfY;
    static constexpr double s_expCavernPosZ = s_ecn3EndZ - s_expCavernHalfZ - s_cavernPosZ;

    // Spectrometer yoke pit: 8.4 m wide, 1 m deep, 9 m long, centred on the magnet
    static constexpr double s_yokePitHalfX = 4.2 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitHalfY = 0.5 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitHalfZ = 4.5 * GeoModelKernelUnits::m;
    static constexpr double s_yokePitPosX = 0.0;
    static constexpr double s_yokePitPosY = s_ecn3FloorY - s_yokePitHalfY;
    static constexpr double s_yokePitPosZ = s_magnetZ - s_cavernPosZ;

    // Target pit: 4 m wide, 1 m deep, 4 m long, ending 2 m downstream of the target
    static constexpr double s_targetPitHalfX = 2.0 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitHalfY = 0.5 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitHalfZ = 2.0 * GeoModelKernelUnits::m;
    static constexpr double s_targetPitPosX = 0.0;
    static constexpr double s_targetPitPosY = s_tcc8FloorY - s_targetPitHalfY;
    static constexpr double s_targetPitPosZ = s_targetEndZ - s_cavernPosZ;
};

}  // namespace SHiPGeometry
