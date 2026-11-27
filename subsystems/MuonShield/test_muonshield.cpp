// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"
#include "SHiP/geometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using SHiP::geometry::Materials;
using SHiP::geometry::MuonShieldFactory;
using SHiP::geometry::units::gm;

namespace {

/// The blocks in placement order: the hadron stopper, then M1-M7.
std::vector<MuonShieldFactory::MagnetData> blocks() {
    std::vector<MuonShieldFactory::MagnetData> all{MuonShieldFactory::k_hadronStopper};
    all.insert(all.end(), MuonShieldFactory::k_magnets.begin(), MuonShieldFactory::k_magnets.end());
    return all;
}

}  // namespace

// The tables are checked against the container at compile time
// (MuonShieldFactory.cpp); this checks the factory builds what they describe.
TEST_CASE("MuonShieldContainerSpansTheShield", "[muonshield]") {
    Materials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);

    const auto* area = dynamic_cast<const GeoBox*>(ms->getLogVol()->getShape());
    REQUIRE(area != nullptr);
    CHECK(area->getXHalfLength() == gm(MuonShieldFactory::s_areaHalfX));
    CHECK(area->getYHalfLength() == gm(MuonShieldFactory::s_areaHalfY));
    CHECK(area->getZHalfLength() == gm(MuonShieldFactory::s_areaHalfZ));
    // M7 is the widest block and the hadron stopper the tallest.
    CHECK(MuonShieldFactory::s_areaHalfX == MuonShieldFactory::k_magnets.back().halfX);
    CHECK(MuonShieldFactory::s_areaHalfY == MuonShieldFactory::k_hadronStopper.halfY);
}

TEST_CASE("MuonShieldBlocksMatchTable", "[muonshield]") {
    Materials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);

    const auto expected = blocks();
    REQUIRE(ms->getNChildVols() == expected.size());
    for (unsigned int i = 0; i < ms->getNChildVols(); ++i) {
        const auto& magnet = expected.at(i);
        INFO("block " << magnet.name);
        const GeoVPhysVol* child = &*ms->getChildVol(i);
        CHECK(child->getLogVol()->getName() == "/SHiP/muon_shield/" + std::string(magnet.name));
        CHECK(child->getLogVol()->getMaterial()->getName() == "Iron");
        CHECK(ms->getXToChildVol(i).translation().z() ==
              gm(magnet.zCentre() - MuonShieldFactory::s_centreZ));
        const auto* box = dynamic_cast<const GeoBox*>(child->getLogVol()->getShape());
        REQUIRE(box != nullptr);
        CHECK(box->getXHalfLength() == gm(magnet.halfX));
        CHECK(box->getYHalfLength() == gm(magnet.halfY));
        CHECK(box->getZHalfLength() == gm(magnet.halfZ));
    }
}
