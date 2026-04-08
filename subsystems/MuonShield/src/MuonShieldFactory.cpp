// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <string>

namespace SHiPGeometry {

// ---------------------------------------------------------------------------
// GDML-derived station data (all dimensions in mm)
// Bounding-box centres and half-sizes are computed from the arb8 vertices in
// ship_geometry.gdml. Station z-positions are relative to the MuonShieldArea
// container centre (GDML station_z_cm - 1676.33 cm) × 10 mm/cm.
// ---------------------------------------------------------------------------
const MuonShieldFactory::StationData MuonShieldFactory::k_stations[6] = {
    // ── MagnAbsorb  (GDML z = 319.5 cm, dz = 115.5 cm) ──────────────────
    {"magn_absorb",
     -13568.3,
     1020.0,
     1691.0,
     1155.0,
     {
         {250.0, 1690.0, 1155.0, 250.0, -1.0, "middle_mag_l"},
         {250.0, 1690.0, 1155.0, -250.0, 1.0, "middle_mag_r"},
         {250.0, 1690.0, 1155.0, 770.0, 0.0, "mag_ret_l"},
         {250.0, 1690.0, 1155.0, -770.0, 0.0, "mag_ret_r"},
         {510.0, 250.0, 1155.0, 510.0, 1440.0, "mag_top_left"},
         {510.0, 250.0, 1155.0, -510.0, 1440.0, "mag_top_right"},
         {510.0, 250.0, 1155.0, 510.0, -1440.0, "mag_bot_left"},
         {510.0, 250.0, 1155.0, -510.0, -1440.0, "mag_bot_right"},
     }},

    // ── Magn1  (GDML z = 950 cm, dz = 495 cm) ────────────────────────────
    {"magn_1",
     -7263.3,
     1697.0,
     1230.0,
     4950.0,
     {
         {399.6, 1228.2, 4950.0, 399.6, 0.0, "middle_mag_l"},
         {399.6, 1228.2, 4950.0, -399.6, 0.0, "middle_mag_r"},
         {487.7, 1229.2, 4950.0, 1208.7, 0.0, "mag_ret_l"},
         {487.7, 1229.2, 4950.0, -1208.7, 0.0, "mag_ret_r"},
         {848.2, 479.6, 4950.0, 848.2, 749.6, "mag_top_left"},
         {848.2, 479.6, 4950.0, -848.2, 749.6, "mag_top_right"},
         {848.2, 479.6, 4950.0, 848.2, -749.6, "mag_bot_left"},
         {848.2, 479.6, 4950.0, -848.2, -749.6, "mag_bot_right"},
     }},

    // ── Magn2  (GDML z = 1735.48 cm, dz = 280.48 cm) ─────────────────────
    {"magn_2",
     591.5,
     1736.0,
     1056.0,
     2804.8,
     {
         {265.6, 1054.6, 2804.8, 265.6, 0.0, "middle_mag_l"},
         {265.6, 1054.6, 2804.8, -265.6, 0.0, "middle_mag_r"},
         {594.7, 1055.6, 2804.8, 1140.3, 0.0, "mag_ret_l"},
         {594.7, 1055.6, 2804.8, -1140.3, 0.0, "mag_ret_r"},
         {867.5, 312.8, 2804.8, 867.5, 742.8, "mag_top_left"},
         {867.5, 312.8, 2804.8, -867.5, 742.8, "mag_top_right"},
         {867.5, 312.8, 2804.8, 867.5, -742.8, "mag_bot_left"},
         {867.5, 312.8, 2804.8, -867.5, -742.8, "mag_bot_right"},
     }},

    // ── Magn3  (GDML z = 2258.49 cm, dz = 232.53 cm) ─────────────────────
    {"magn_3",
     5821.6,
     1781.0,
     597.0,
     2325.3,
     {
         {18.4, 595.8, 2325.3, 23.4, 0.0, "middle_mag_l"},
         {18.4, 595.8, 2325.3, -23.4, 0.0, "middle_mag_r"},
         {849.3, 596.8, 2325.3, 931.6, 0.0, "mag_ret_l"},
         {849.3, 596.8, 2325.3, -931.6, 0.0, "mag_ret_r"},
         {888.0, 18.4, 2325.3, 893.0, 578.4, "mag_top_left"},
         {888.0, 18.4, 2325.3, -893.0, 578.4, "mag_top_right"},
         {888.0, 18.4, 2325.3, 893.0, -578.4, "mag_bot_left"},
         {888.0, 18.4, 2325.3, -893.0, -578.4, "mag_bot_right"},
     }},

    // ── Magn4  (GDML z = 2586.02 cm, dz = 85 cm) ─────────────────────────
    {"magn_4",
     9096.9,
     1797.0,
     1332.0,
     850.0,
     {
         {535.6, 1330.2, 850.0, 535.6, 0.0, "middle_mag_l"},
         {535.6, 1330.2, 850.0, -535.6, 0.0, "middle_mag_r"},
         {713.0, 1331.2, 850.0, 1083.0, 0.0, "mag_ret_l"},
         {713.0, 1331.2, 850.0, -1083.0, 0.0, "mag_ret_r"},
         {898.0, 385.6, 850.0, 898.0, 945.6, "mag_top_left"},
         {898.0, 385.6, 850.0, -898.0, 945.6, "mag_top_right"},
         {898.0, 385.6, 850.0, 898.0, -945.6, "mag_bot_left"},
         {898.0, 385.6, 850.0, -898.0, -945.6, "mag_bot_right"},
     }},

    // ── Magn5  (GDML z = 2914.84 cm, dz = 233.82 cm) ─────────────────────
    {"magn_5",
     12385.1,
     1808.0,
     960.0,
     2338.2,
     {
         {200.0, 959.0, 2338.2, 200.0, 0.0, "middle_mag_l"},
         {200.0, 959.0, 2338.2, -200.0, 0.0, "middle_mag_r"},
         {728.9, 960.0, 2338.2, 1079.2, 0.0, "mag_ret_l"},
         {728.9, 960.0, 2338.2, -1079.2, 0.0, "mag_ret_r"},
         {904.0, 200.0, 2338.2, 904.0, 760.0, "mag_top_left"},
         {904.0, 200.0, 2338.2, -904.0, 760.0, "mag_top_right"},
         {904.0, 200.0, 2338.2, 904.0, -760.0, "mag_bot_left"},
         {904.0, 200.0, 2338.2, -904.0, -760.0, "mag_bot_right"},
     }},
};

// ---------------------------------------------------------------------------

MuonShieldFactory::MuonShieldFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* MuonShieldFactory::buildStation(const StationData& station) {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    // Air container that spans all 8 pieces of this station
    auto* stationBox =
        new GeoBox(station.containerHalfX, station.containerHalfY, station.containerHalfZ);
    std::string containerName = "/SHiP/muon_shield/" + std::string(station.name);
    auto* stationLog = new GeoLogVol(containerName, stationBox, air);
    auto* stationPhys = new GeoPhysVol(stationLog);

    // Place 8 Iron bounding-box approximations
    for (const PieceData& piece : station.pieces) {
        auto* pieceBox = new GeoBox(piece.halfX, piece.halfY, piece.halfZ);
        std::string pieceName = "/SHiP/muon_shield/" + std::string(station.name) + "/" + piece.name;
        auto* pieceLog = new GeoLogVol(pieceName, pieceBox, iron);
        auto* piecePhys = new GeoPhysVol(pieceLog);

        GeoTrf::Transform3D trf = GeoTrf::Translate3D(piece.centX, piece.centY, 0.0);
        stationPhys->add(new GeoNameTag(pieceName));
        stationPhys->add(new GeoIdentifierTag(static_cast<int>(&piece - &station.pieces[0])));
        stationPhys->add(new GeoTransform(trf));
        stationPhys->add(piecePhys);
    }

    return stationPhys;
}

GeoPhysVol* MuonShieldFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Overall MuonShieldArea container (Air)
    auto* areaBox = new GeoBox(s_areaHalfX, s_areaHalfY, s_areaHalfZ);
    auto* areaLog = new GeoLogVol("/SHiP/muon_shield", areaBox, air);
    auto* areaPhys = new GeoPhysVol(areaLog);

    // Build and place 6 stations
    for (const StationData& station : k_stations) {
        GeoPhysVol* stationPhys = buildStation(station);
        std::string stationName = "/SHiP/muon_shield/" + std::string(station.name);
        GeoTrf::Transform3D trf = GeoTrf::Translate3D(0.0, 0.0, station.stationZ);
        areaPhys->add(new GeoNameTag(stationName));
        areaPhys->add(new GeoIdentifierTag(static_cast<int>(&station - &k_stations[0])));
        areaPhys->add(new GeoTransform(trf));
        areaPhys->add(stationPhys);
    }

    return areaPhys;
}

}  // namespace SHiPGeometry
