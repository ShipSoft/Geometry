// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"
#include "SHiPGeometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <iterator>
#include <string>

using SHiPGeometry::MuonShieldFactory;
using SHiPGeometry::SHiPMaterials;
using SHiPGeometry::units::gm;

// The station table is checked against the container at compile time
// (MuonShieldFactory.cpp); this checks the factory builds what it describes.
TEST_CASE("MuonShieldStationsMatchTable", "[muonshield]") {
    SHiPMaterials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    REQUIRE(ms->getNChildVols() == std::size(MuonShieldFactory::k_stations));

    for (unsigned int i = 0; i < ms->getNChildVols(); ++i) {
        const auto& station = MuonShieldFactory::k_stations[i];
        INFO("station " << station.name);
        const GeoVPhysVol* child = &*ms->getChildVol(i);
        CHECK(child->getLogVol()->getName() == "/SHiP/muon_shield/" + std::string(station.name));
        CHECK(ms->getXToChildVol(i).translation().z() == gm(station.stationZ));
        auto* box = dynamic_cast<const GeoBox*>(child->getLogVol()->getShape());
        REQUIRE(box != nullptr);
        CHECK(box->getXHalfLength() == gm(station.containerHalfX));
        CHECK(box->getYHalfLength() == gm(station.containerHalfY));
        CHECK(box->getZHalfLength() == gm(station.containerHalfZ));
        CHECK(child->getNChildVols() == std::size(station.pieces));
    }
}
