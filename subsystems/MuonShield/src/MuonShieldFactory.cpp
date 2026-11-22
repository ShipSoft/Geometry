// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <string>
#include <string_view>

namespace SHiP::geometry {

using units::gm;
using units::mm;

// ---------------------------------------------------------------------------
// GDML-derived station data (all dimensions in mm)
// Bounding-box centres and half-sizes are computed from the arb8 vertices in
// ship_geometry.gdml. Station z-positions are relative to the MuonShieldArea
// container centre (GDML station_z_cm - 1676.33 cm) × 10 mm/cm.
// ---------------------------------------------------------------------------
constexpr MuonShieldFactory::StationData
    MuonShieldFactory::k_stations[6] =
        {
            // ── MagnAbsorb  (GDML z = 319.5 cm, dz = 115.5 cm) ──────────────────
            {.name = "magn_absorb",
             .stationZ = -13568.3 * mm,
             .containerHalfX = 1020.0 * mm,
             .containerHalfY = 1691.0 * mm,
             .containerHalfZ = 1155.0 * mm,
             .pieces =
                 {
                     {.halfX = 250.0 * mm,
                      .halfY = 1690.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = 250.0 * mm,
                      .centY = -1.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 250.0 * mm,
                      .halfY = 1690.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = -250.0 * mm,
                      .centY = 1.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 250.0 * mm,
                      .halfY = 1690.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = 770.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 250.0 * mm,
                      .halfY = 1690.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = -770.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 510.0 * mm,
                      .halfY = 250.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = 510.0 * mm,
                      .centY = 1440.0 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 510.0 * mm,
                      .halfY = 250.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = -510.0 * mm,
                      .centY = 1440.0 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 510.0 * mm,
                      .halfY = 250.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = 510.0 * mm,
                      .centY = -1440.0 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 510.0 * mm,
                      .halfY = 250.0 * mm,
                      .halfZ = 1155.0 * mm,
                      .centX = -510.0 * mm,
                      .centY = -1440.0 * mm,
                      .name = "mag_bot_right"},
                 }},

            // ── Magn1  (GDML z = 950 cm, dz = 495 cm) ────────────────────────────
            {.name = "magn_1",
             .stationZ = -7263.3 * mm,
             .containerHalfX = 1697.0 * mm,
             .containerHalfY = 1230.0 * mm,
             .containerHalfZ = 4950.0 * mm,
             .pieces =
                 {
                     {.halfX = 399.6 * mm,
                      .halfY = 1228.2 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = 399.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 399.6 * mm,
                      .halfY = 1228.2 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = -399.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 487.7 * mm,
                      .halfY = 1229.2 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = 1208.7 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 487.7 * mm,
                      .halfY = 1229.2 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = -1208.7 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 848.2 * mm,
                      .halfY = 479.6 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = 848.2 * mm,
                      .centY = 749.6 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 848.2 * mm,
                      .halfY = 479.6 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = -848.2 * mm,
                      .centY = 749.6 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 848.2 * mm,
                      .halfY = 479.6 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = 848.2 * mm,
                      .centY = -749.6 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 848.2 * mm,
                      .halfY = 479.6 * mm,
                      .halfZ = 4950.0 * mm,
                      .centX = -848.2 * mm,
                      .centY = -749.6 * mm,
                      .name = "mag_bot_right"},
                 }},

            // ── Magn2  (GDML z = 1735.48 cm, dz = 280.48 cm) ─────────────────────
            {.name = "magn_2",
             .stationZ = 591.5 * mm,
             .containerHalfX = 1736.0 * mm,
             .containerHalfY = 1056.0 * mm,
             .containerHalfZ = 2804.8 * mm,
             .pieces =
                 {
                     {.halfX = 265.6 * mm,
                      .halfY = 1054.6 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = 265.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 265.6 * mm,
                      .halfY = 1054.6 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = -265.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 594.7 * mm,
                      .halfY = 1055.6 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = 1140.3 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 594.7 * mm,
                      .halfY = 1055.6 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = -1140.3 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 867.5 * mm,
                      .halfY = 312.8 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = 867.5 * mm,
                      .centY = 742.8 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 867.5 * mm,
                      .halfY = 312.8 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = -867.5 * mm,
                      .centY = 742.8 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 867.5 * mm,
                      .halfY = 312.8 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = 867.5 * mm,
                      .centY = -742.8 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 867.5 * mm,
                      .halfY = 312.8 * mm,
                      .halfZ = 2804.8 * mm,
                      .centX = -867.5 * mm,
                      .centY = -742.8 * mm,
                      .name = "mag_bot_right"},
                 }},

            // ── Magn3  (GDML z = 2258.49 cm, dz = 232.53 cm) ─────────────────────
            {.name = "magn_3",
             .stationZ = 5821.6 * mm,
             .containerHalfX = 1781.0 * mm,
             .containerHalfY = 597.0 * mm,
             .containerHalfZ = 2325.3 * mm,
             .pieces =
                 {
                     {.halfX = 18.4 * mm,
                      .halfY = 595.8 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = 23.4 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 18.4 * mm,
                      .halfY = 595.8 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = -23.4 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 849.3 * mm,
                      .halfY = 596.8 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = 931.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 849.3 * mm,
                      .halfY = 596.8 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = -931.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 888.0 * mm,
                      .halfY = 18.4 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = 893.0 * mm,
                      .centY = 578.4 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 888.0 * mm,
                      .halfY = 18.4 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = -893.0 * mm,
                      .centY = 578.4 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 888.0 * mm,
                      .halfY = 18.4 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = 893.0 * mm,
                      .centY = -578.4 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 888.0 * mm,
                      .halfY = 18.4 * mm,
                      .halfZ = 2325.3 * mm,
                      .centX = -893.0 * mm,
                      .centY = -578.4 * mm,
                      .name = "mag_bot_right"},
                 }},

            // ── Magn4  (GDML z = 2586.02 cm, dz = 85 cm) ─────────────────────────
            {.name = "magn_4",
             .stationZ = 9096.9 * mm,
             .containerHalfX = 1797.0 * mm,
             .containerHalfY = 1332.0 * mm,
             .containerHalfZ = 850.0 * mm,
             .pieces =
                 {
                     {.halfX = 535.6 * mm,
                      .halfY = 1330.2 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = 535.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 535.6 * mm,
                      .halfY = 1330.2 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = -535.6 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 713.0 * mm,
                      .halfY = 1331.2 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = 1083.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 713.0 * mm,
                      .halfY = 1331.2 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = -1083.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 898.0 * mm,
                      .halfY = 385.6 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = 898.0 * mm,
                      .centY = 945.6 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 898.0 * mm,
                      .halfY = 385.6 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = -898.0 * mm,
                      .centY = 945.6 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 898.0 * mm,
                      .halfY = 385.6 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = 898.0 * mm,
                      .centY = -945.6 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 898.0 * mm,
                      .halfY = 385.6 * mm,
                      .halfZ = 850.0 * mm,
                      .centX = -898.0 * mm,
                      .centY = -945.6 * mm,
                      .name = "mag_bot_right"},
                 }},

            // ── Magn5  (GDML z = 2914.84 cm, dz = 233.82 cm) ─────────────────────
            {.name = "magn_5",
             .stationZ = 12385.1 * mm,
             .containerHalfX = 1808.0 * mm,
             .containerHalfY = 960.0 * mm,
             .containerHalfZ = 2338.2 * mm,
             .pieces =
                 {
                     {.halfX = 200.0 * mm,
                      .halfY = 959.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = 200.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_l"},
                     {.halfX = 200.0 * mm,
                      .halfY = 959.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = -200.0 * mm,
                      .centY = 0.0 * mm,
                      .name = "middle_mag_r"},
                     {.halfX = 728.9 * mm,
                      .halfY = 960.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = 1079.2 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_l"},
                     {.halfX = 728.9 * mm,
                      .halfY = 960.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = -1079.2 * mm,
                      .centY = 0.0 * mm,
                      .name = "mag_ret_r"},
                     {.halfX = 904.0 * mm,
                      .halfY = 200.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = 904.0 * mm,
                      .centY = 760.0 * mm,
                      .name = "mag_top_left"},
                     {.halfX = 904.0 * mm,
                      .halfY = 200.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = -904.0 * mm,
                      .centY = 760.0 * mm,
                      .name = "mag_top_right"},
                     {.halfX = 904.0 * mm,
                      .halfY = 200.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = 904.0 * mm,
                      .centY = -760.0 * mm,
                      .name = "mag_bot_left"},
                     {.halfX = 904.0 * mm,
                      .halfY = 200.0 * mm,
                      .halfZ = 2338.2 * mm,
                      .centX = -904.0 * mm,
                      .centY = -760.0 * mm,
                      .name = "mag_bot_right"},
                 }},
};

// ── Compile-time validation ──────────────────────────────────────────────
namespace {

using MSF = MuonShieldFactory;

constexpr checks::Box stationBox(const MSF::StationData& st) {
    return checks::box(0.0 * mm, 0.0 * mm, st.stationZ, st.containerHalfX, st.containerHalfY,
                       st.containerHalfZ);
}

// Pieces are placed in the station's XY frame at z = 0.
constexpr bool pieceFits(const MSF::StationData& st, const MSF::PieceData& p,
                         units::LengthMm tol = 0.0 * mm) {
    return checks::contains(checks::box(st.containerHalfX, st.containerHalfY, st.containerHalfZ),
                            checks::box(p.centX, p.centY, 0.0 * mm, p.halfX, p.halfY, p.halfZ),
                            tol);
}

// Known deviation (#120): the magn_5 return-yoke pieces (1079.2 + 728.9 mm)
// reach 0.1 mm past the station container half-width (1808.0 mm). They may
// stick out by no more than kKnownProtrusion (0.1 mm plus rounding).
constexpr auto kKnownProtrusion = 0.11 * mm;

constexpr bool isKnownProtrusion(const MSF::StationData& st, const MSF::PieceData& p) {
    const std::string_view station = st.name;
    const std::string_view piece = p.name;
    return station == "magn_5" && (piece == "mag_ret_l" || piece == "mag_ret_r");
}

constexpr bool piecesFitStations() {
    for (const auto& st : MSF::k_stations) {
        for (const auto& p : st.pieces) {
            const bool known = isKnownProtrusion(st, p);
            if (pieceFits(st, p) == known || (known && !pieceFits(st, p, kKnownProtrusion))) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

static_assert(piecesFitStations(),
              "a muon-shield piece sticks out of its station container, or a known "
              "protrusion has been fixed (then drop it from isKnownProtrusion) or has grown "
              "past kKnownProtrusion");
static_assert(checks::firstFailure(MSF::k_stations,
                                   [](const MSF::StationData& st) {
                                       return checks::contains(
                                           checks::box(MSF::s_areaHalfX, MSF::s_areaHalfY,
                                                       MSF::s_areaHalfZ),
                                           stationBox(st));
                                   }) == checks::npos,
              "a muon-shield station sticks out of the MuonShieldArea container");
static_assert(checks::firstPairFailure(MSF::k_stations,
                                       [](const MSF::StationData& a, const MSF::StationData& b) {
                                           return stationBox(a).z.hi <= stationBox(b).z.lo;
                                       }) == checks::npos,
              "muon-shield stations must be ordered along z without overlapping");

// ---------------------------------------------------------------------------

MuonShieldFactory::MuonShieldFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* MuonShieldFactory::buildStation(const StationData& station) {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    // Air container that spans all 8 pieces of this station
    auto const* stationBox = new GeoBox(gm(station.containerHalfX), gm(station.containerHalfY),
                                        gm(station.containerHalfZ));
    const std::string containerName = "/SHiP/muon_shield/" + std::string(station.name);
    const auto* stationLog = new GeoLogVol(containerName, stationBox, air);
    auto* stationPhys = new GeoPhysVol(stationLog);

    // Place 8 Iron bounding-box approximations
    for (const PieceData& piece : station.pieces) {
        auto const* pieceBox = new GeoBox(gm(piece.halfX), gm(piece.halfY), gm(piece.halfZ));
        const std::string pieceName =
            "/SHiP/muon_shield/" + std::string(station.name) + "/" + piece.name;
        const auto* pieceLog = new GeoLogVol(pieceName, pieceBox, iron);
        auto* piecePhys = new GeoPhysVol(pieceLog);

        GeoTrf::Transform3D const trf = GeoTrf::Translate3D(gm(piece.centX), gm(piece.centY), 0.0);
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
    auto const* areaBox = new GeoBox(gm(s_areaHalfX), gm(s_areaHalfY), gm(s_areaHalfZ));
    const auto* areaLog = new GeoLogVol("/SHiP/muon_shield", areaBox, air);
    auto* areaPhys = new GeoPhysVol(areaLog);

    // Build and place 6 stations
    for (const StationData& station : k_stations) {
        GeoPhysVol* stationPhys = buildStation(station);
        const std::string stationName = "/SHiP/muon_shield/" + std::string(station.name);
        GeoTrf::Transform3D const trf = GeoTrf::Translate3D(0.0, 0.0, gm(station.stationZ));
        areaPhys->add(new GeoNameTag(stationName));
        areaPhys->add(new GeoIdentifierTag(static_cast<int>(&station - &k_stations[0])));
        areaPhys->add(new GeoTransform(trf));
        areaPhys->add(stationPhys);
    }

    return areaPhys;
}

REGISTER_SUBSYSTEM(MuonShieldFactory)

}  // namespace SHiP::geometry
