// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the MuonShield (muon shield magnets) geometry
 *
 * Creates a MuonShieldArea (Air) container holding 6 magnet stations.
 * Each station contains 8 Iron pieces — bounding-box approximations of the
 * GDML arb8 shapes. Station z-positions (from GDML MuonShieldArea origin, cm):
 *   MagnAbsorb: 319.5, Magn1: 950, Magn2: 1735.48,
 *   Magn3: 2258.49, Magn4: 2586.02, Magn5: 2914.84
 */
class MuonShieldFactory {
   public:
    explicit MuonShieldFactory(SHiPMaterials& materials);
    ~MuonShieldFactory() = default;

    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    struct PieceData {
        double halfX, halfY, halfZ;  // bounding-box half-sizes (mm)
        double centX, centY;         // centre offset in station XY frame (mm)
        const char* name;            // piece name suffix
    };

    struct StationData {
        const char* name;  // station name
        double stationZ;   // z in MuonShieldArea frame (mm)
        double containerHalfX;
        double containerHalfY;
        double containerHalfZ;
        PieceData pieces[8];
    };

    // GDML-derived station data for all 6 stations × 8 pieces
    static const StationData k_stations[6];

    GeoPhysVol* buildStation(const StationData& station);

    // MuonShieldArea container dimensions (mm)
    static constexpr double s_areaHalfX = 1810.0;
    static constexpr double s_areaHalfY = 1700.0;
    static constexpr double s_areaHalfZ = 14724.0;
};

}  // namespace SHiPGeometry
