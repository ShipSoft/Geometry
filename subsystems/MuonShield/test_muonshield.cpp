// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"
#include "SHiP/geometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <string_view>
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
TEST_CASE("MuonShieldContainerSpansTheShieldAroundTheSnd", "[muonshield]") {
    Materials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);

    // The container is a box with the SND's sections cut out of it, one cut
    // per cavity step, so that the SND does not overlap it.
    const GeoShape* shape = ms->getLogVol()->getShape();
    std::size_t nCuts = 0;
    while (const auto* cut = dynamic_cast<const GeoShapeSubtraction*>(shape)) {
        shape = cut->getOpA();
        ++nCuts;
    }
    CHECK(nCuts == MuonShieldFactory::k_sndCavity.size());
    const auto* area = dynamic_cast<const GeoBox*>(shape);
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

        // Magnets with the SND cavity are a box minus one cut per cavity step
        // inside them; the others are boxes.
        const GeoShape* shape = child->getLogVol()->getShape();
        std::size_t nCuts = 0;
        while (const auto* cut = dynamic_cast<const GeoShapeSubtraction*>(shape)) {
            shape = cut->getOpA();
            ++nCuts;
        }
        CHECK(nCuts == static_cast<std::size_t>(std::ranges::count_if(
                           MuonShieldFactory::k_sndCavity,
                           [&magnet](const MuonShieldFactory::CavitySection& section) {
                               return MuonShieldFactory::cuts(section, magnet);
                           })));
        const auto* box = dynamic_cast<const GeoBox*>(shape);
        REQUIRE(box != nullptr);
        CHECK(box->getXHalfLength() == gm(magnet.halfX));
        CHECK(box->getYHalfLength() == gm(magnet.halfY));
        CHECK(box->getZHalfLength() == gm(magnet.halfZ));
    }
}

// M5 holds the veto, target and MGC steps of the cavity, M6 only the HCAL
// step, which spans it face to face.
TEST_CASE("MuonShieldCavityStepsPerMagnet", "[muonshield]") {
    const auto& m5 = MuonShieldFactory::k_magnets.at(4);
    const auto& m6 = MuonShieldFactory::k_magnets.at(5);
    const auto& cavity = MuonShieldFactory::k_sndCavity;
    REQUIRE(cavity.size() == 4);
    for (std::size_t i = 0; i < cavity.size(); ++i) {
        INFO("cavity step " << i);
        CHECK(MuonShieldFactory::cuts(cavity.at(i), m5) == (i < 3));
        CHECK(MuonShieldFactory::cuts(cavity.at(i), m6) == (i == 3));
    }
    CHECK(cavity.back().zStart == m6.zStart);
    CHECK(cavity.back().zEnd == m6.zEnd());
    for (const auto& magnet : blocks()) {
        INFO("block " << magnet.name);
        const std::string_view name = magnet.name;
        CHECK(MuonShieldFactory::hasSndCavity(magnet) == (name == "magn_5" || name == "magn_6"));
    }
}
