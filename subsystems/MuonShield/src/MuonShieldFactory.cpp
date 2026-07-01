// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SubsystemRegistry.h"

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
     -13568.3 * mm,
     1020.0 * mm,
     1691.0 * mm,
     1155.0 * mm,
     {
         {250.0 * mm, 1690.0 * mm, 1155.0 * mm, 250.0 * mm, -1.0 * mm, "middle_mag_l"},
         {250.0 * mm, 1690.0 * mm, 1155.0 * mm, -250.0 * mm, 1.0 * mm, "middle_mag_r"},
         {250.0 * mm, 1690.0 * mm, 1155.0 * mm, 770.0 * mm, 0.0, "mag_ret_l"},
         {250.0 * mm, 1690.0 * mm, 1155.0 * mm, -770.0 * mm, 0.0, "mag_ret_r"},
         {510.0 * mm, 250.0 * mm, 1155.0 * mm, 510.0 * mm, 1440.0 * mm, "mag_top_left"},
         {510.0 * mm, 250.0 * mm, 1155.0 * mm, -510.0 * mm, 1440.0 * mm, "mag_top_right"},
         {510.0 * mm, 250.0 * mm, 1155.0 * mm, 510.0 * mm, -1440.0 * mm, "mag_bot_left"},
         {510.0 * mm, 250.0 * mm, 1155.0 * mm, -510.0 * mm, -1440.0 * mm, "mag_bot_right"},
     }},

    // ── Magn1  (GDML z = 950 cm, dz = 495 cm) ────────────────────────────
    {"magn_1",
     -7263.3 * mm,
     1697.0 * mm,
     1230.0 * mm,
     4950.0 * mm,
     {
         {399.6 * mm, 1228.2 * mm, 4950.0 * mm, 399.6 * mm, 0.0, "middle_mag_l"},
         {399.6 * mm, 1228.2 * mm, 4950.0 * mm, -399.6 * mm, 0.0, "middle_mag_r"},
         {487.7 * mm, 1229.2 * mm, 4950.0 * mm, 1208.7 * mm, 0.0, "mag_ret_l"},
         {487.7 * mm, 1229.2 * mm, 4950.0 * mm, -1208.7 * mm, 0.0, "mag_ret_r"},
         {848.2 * mm, 479.6 * mm, 4950.0 * mm, 848.2 * mm, 749.6 * mm, "mag_top_left"},
         {848.2 * mm, 479.6 * mm, 4950.0 * mm, -848.2 * mm, 749.6 * mm, "mag_top_right"},
         {848.2 * mm, 479.6 * mm, 4950.0 * mm, 848.2 * mm, -749.6 * mm, "mag_bot_left"},
         {848.2 * mm, 479.6 * mm, 4950.0 * mm, -848.2 * mm, -749.6 * mm, "mag_bot_right"},
     }},

    // ── Magn2  (GDML z = 1735.48 cm, dz = 280.48 cm) ─────────────────────
    {"magn_2",
     591.5 * mm,
     1736.0 * mm,
     1056.0 * mm,
     2804.8 * mm,
     {
         {265.6 * mm, 1054.6 * mm, 2804.8 * mm, 265.6 * mm, 0.0, "middle_mag_l"},
         {265.6 * mm, 1054.6 * mm, 2804.8 * mm, -265.6 * mm, 0.0, "middle_mag_r"},
         {594.7 * mm, 1055.6 * mm, 2804.8 * mm, 1140.3 * mm, 0.0, "mag_ret_l"},
         {594.7 * mm, 1055.6 * mm, 2804.8 * mm, -1140.3 * mm, 0.0, "mag_ret_r"},
         {867.5 * mm, 312.8 * mm, 2804.8 * mm, 867.5 * mm, 742.8 * mm, "mag_top_left"},
         {867.5 * mm, 312.8 * mm, 2804.8 * mm, -867.5 * mm, 742.8 * mm, "mag_top_right"},
         {867.5 * mm, 312.8 * mm, 2804.8 * mm, 867.5 * mm, -742.8 * mm, "mag_bot_left"},
         {867.5 * mm, 312.8 * mm, 2804.8 * mm, -867.5 * mm, -742.8 * mm, "mag_bot_right"},
     }},

    // ── Magn3  (GDML z = 2258.49 cm, dz = 232.53 cm) ─────────────────────
    {"magn_3",
     5821.6 * mm,
     1781.0 * mm,
     597.0 * mm,
     2325.3 * mm,
     {
         {18.4 * mm, 595.8 * mm, 2325.3 * mm, 23.4 * mm, 0.0, "middle_mag_l"},
         {18.4 * mm, 595.8 * mm, 2325.3 * mm, -23.4 * mm, 0.0, "middle_mag_r"},
         {849.3 * mm, 596.8 * mm, 2325.3 * mm, 931.6 * mm, 0.0, "mag_ret_l"},
         {849.3 * mm, 596.8 * mm, 2325.3 * mm, -931.6 * mm, 0.0, "mag_ret_r"},
         {888.0 * mm, 18.4 * mm, 2325.3 * mm, 893.0 * mm, 578.4 * mm, "mag_top_left"},
         {888.0 * mm, 18.4 * mm, 2325.3 * mm, -893.0 * mm, 578.4 * mm, "mag_top_right"},
         {888.0 * mm, 18.4 * mm, 2325.3 * mm, 893.0 * mm, -578.4 * mm, "mag_bot_left"},
         {888.0 * mm, 18.4 * mm, 2325.3 * mm, -893.0 * mm, -578.4 * mm, "mag_bot_right"},
     }},

    // ── Magn4  (GDML z = 2586.02 cm, dz = 85 cm) ─────────────────────────
    {"magn_4",
     9096.9 * mm,
     1797.0 * mm,
     1332.0 * mm,
     850.0 * mm,
     {
         {535.6 * mm, 1330.2 * mm, 850.0 * mm, 535.6 * mm, 0.0, "middle_mag_l"},
         {535.6 * mm, 1330.2 * mm, 850.0 * mm, -535.6 * mm, 0.0, "middle_mag_r"},
         {713.0 * mm, 1331.2 * mm, 850.0 * mm, 1083.0 * mm, 0.0, "mag_ret_l"},
         {713.0 * mm, 1331.2 * mm, 850.0 * mm, -1083.0 * mm, 0.0, "mag_ret_r"},
         {898.0 * mm, 385.6 * mm, 850.0 * mm, 898.0 * mm, 945.6 * mm, "mag_top_left"},
         {898.0 * mm, 385.6 * mm, 850.0 * mm, -898.0 * mm, 945.6 * mm, "mag_top_right"},
         {898.0 * mm, 385.6 * mm, 850.0 * mm, 898.0 * mm, -945.6 * mm, "mag_bot_left"},
         {898.0 * mm, 385.6 * mm, 850.0 * mm, -898.0 * mm, -945.6 * mm, "mag_bot_right"},
     }},

    // ── Magn5  (GDML z = 2914.84 cm, dz = 233.82 cm) ─────────────────────
    {"magn_5",
     12385.1 * mm,
     1808.0 * mm,
     960.0 * mm,
     2338.2 * mm,
     {
         {200.0 * mm, 959.0 * mm, 2338.2 * mm, 200.0 * mm, 0.0, "middle_mag_l"},
         {200.0 * mm, 959.0 * mm, 2338.2 * mm, -200.0 * mm, 0.0, "middle_mag_r"},
         {728.9 * mm, 960.0 * mm, 2338.2 * mm, 1079.2 * mm, 0.0, "mag_ret_l"},
         {728.9 * mm, 960.0 * mm, 2338.2 * mm, -1079.2 * mm, 0.0, "mag_ret_r"},
         {904.0 * mm, 200.0 * mm, 2338.2 * mm, 904.0 * mm, 760.0 * mm, "mag_top_left"},
         {904.0 * mm, 200.0 * mm, 2338.2 * mm, -904.0 * mm, 760.0 * mm, "mag_top_right"},
         {904.0 * mm, 200.0 * mm, 2338.2 * mm, 904.0 * mm, -760.0 * mm, "mag_bot_left"},
         {904.0 * mm, 200.0 * mm, 2338.2 * mm, -904.0 * mm, -760.0 * mm, "mag_bot_right"},
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

REGISTER_SUBSYSTEM(MuonShieldFactory)

}  // namespace SHiPGeometry
