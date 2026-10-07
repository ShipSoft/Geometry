// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the Cavern (world volume and rock) geometry
 */
class CavernFactory {
   public:
    explicit CavernFactory(Materials& materials);
    ~CavernFactory() = default;

    /**
     * @brief Build the Cavern geometry
     * @return Pointer to the world physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

   private:
    Materials& m_materials;

    GeoPhysVol* m_world{nullptr};
    GeoPhysVol* m_cavern{nullptr};

    // World volume dimensions (half-sizes in mm)
    static constexpr double s_worldHalfX = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfY = 200.0 * GeoModelKernelUnits::m;
    static constexpr double s_worldHalfZ = 200.0 * GeoModelKernelUnits::m;

    // ── Layout (global frame: origin at the target front face, mm) ──────
    //
    // Inner concrete surfaces of the civil-engineering solid in the CAD model
    // ST1967028_01, which underlies the integration layout SPSXXSHIP0002
    // version 2026-0.1 (EDMS 3287817 v1.1), sectioned across the beam:
    //  - TCC8: floor 1.703 m below the beam, walls at x = -3.587 m and
    //    +6.413 m, flat ceiling at y = 5.761 m. Upstream of z = 6.2 m the
    //    tunnel is arched instead; the arch is not modelled. The floor ends
    //    at z = 21.37 m.
    //  - Step: a floor 0.80 m lower at the TCC8 width, z = 21.37 m to 22.19 m.
    //  - ECN3: floor 3.364 m below the beam, walls at x = -4.566 m and
    //    +11.434 m (mid-hall; the walls drift by 0.35 mrad against the beam),
    //    from z = 22.19 m to the end wall at 122.18 m. The hall is modelled up
    //    to the arch springline at y = 8.636 m; the 0.5 m crane ledges above
    //    y = 5.84 m and the arch itself (crown at y = 11.586 m) are not.
    // Neither pit is in the civil model; both are carried over from the
    // FairShip cavern (ShipCave).
    static constexpr double s_stepZ = 21.37 * GeoModelKernelUnits::m;  // end of the TCC8 floor
    static constexpr double s_tcc8Length =
        170.0 * GeoModelKernelUnits::m;  // tunnel modelled upstream of the step
    static constexpr double s_stairLength =
        0.82 * GeoModelKernelUnits::m;  // intermediate floor before ECN3
    static constexpr double s_ecn3EndZ =
        122.18 * GeoModelKernelUnits::m;  // downstream end wall of ECN3
    static constexpr double s_tcc8FloorY = -1.703 * GeoModelKernelUnits::m;
    static constexpr double s_tcc8CeilingY = 5.761 * GeoModelKernelUnits::m;
    static constexpr double s_tcc8LeftX = -3.587 * GeoModelKernelUnits::m;
    static constexpr double s_tcc8RightX = 6.413 * GeoModelKernelUnits::m;
    static constexpr double s_stairFloorY = -2.503 * GeoModelKernelUnits::m;
    static constexpr double s_ecn3FloorY = -3.364 * GeoModelKernelUnits::m;
    static constexpr double s_ecn3SpringlineY = 8.636 * GeoModelKernelUnits::m;
    static constexpr double s_ecn3LeftX = -4.566 * GeoModelKernelUnits::m;
    static constexpr double s_ecn3RightX = 11.434 * GeoModelKernelUnits::m;
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

    // TCC8 tunnel (muon shield cavern), ending at the step
    static constexpr double s_muonCavernHalfX = (s_tcc8RightX - s_tcc8LeftX) / 2.0;
    static constexpr double s_muonCavernHalfY = (s_tcc8CeilingY - s_tcc8FloorY) / 2.0;
    static constexpr double s_muonCavernHalfZ = s_tcc8Length / 2.0;
    static constexpr double s_muonCavernPosX = (s_tcc8RightX + s_tcc8LeftX) / 2.0;
    static constexpr double s_muonCavernPosY = (s_tcc8CeilingY + s_tcc8FloorY) / 2.0;
    static constexpr double s_muonCavernPosZ = s_stepZ - s_muonCavernHalfZ - s_cavernPosZ;

    // Stair step: intermediate floor at the TCC8 width and ceiling
    static constexpr double s_stairHalfX = s_muonCavernHalfX;
    static constexpr double s_stairHalfY = (s_tcc8CeilingY - s_stairFloorY) / 2.0;
    static constexpr double s_stairHalfZ = s_stairLength / 2.0;
    static constexpr double s_stairPosX = s_muonCavernPosX;
    static constexpr double s_stairPosY = (s_tcc8CeilingY + s_stairFloorY) / 2.0;
    static constexpr double s_stairPosZ = s_stepZ + s_stairHalfZ - s_cavernPosZ;

    // ECN3 hall (experiment cavern), from the stair step to the end wall
    static constexpr double s_expCavernHalfX = (s_ecn3RightX - s_ecn3LeftX) / 2.0;
    static constexpr double s_expCavernHalfY = (s_ecn3SpringlineY - s_ecn3FloorY) / 2.0;
    static constexpr double s_expCavernHalfZ = (s_ecn3EndZ - s_stepZ - s_stairLength) / 2.0;
    static constexpr double s_expCavernPosX = (s_ecn3RightX + s_ecn3LeftX) / 2.0;
    static constexpr double s_expCavernPosY = (s_ecn3SpringlineY + s_ecn3FloorY) / 2.0;
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

}  // namespace SHiP::geometry
