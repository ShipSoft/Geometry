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

using SHiP::geometry::Materials;
using SHiP::geometry::MuonShieldFactory;
using SHiP::geometry::units::gm;

// The magnet table is checked against the container at compile time
// (MuonShieldFactory.cpp); this checks the factory builds what it describes.
TEST_CASE("MuonShieldMagnetsMatchTable", "[muonshield]") {
    Materials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    REQUIRE(ms->getNChildVols() == MuonShieldFactory::k_magnets.size());

    for (unsigned int i = 0; i < ms->getNChildVols(); ++i) {
        const auto& magnet = MuonShieldFactory::k_magnets.at(i);
        INFO("magnet " << magnet.name);
        const GeoVPhysVol* child = &*ms->getChildVol(i);
        CHECK(child->getLogVol()->getName() == "/SHiP/muon_shield/" + std::string(magnet.name));
        CHECK(child->getLogVol()->getMaterial()->getName() == "Iron");
        CHECK(child->getNChildVols() == 0);
        CHECK(ms->getXToChildVol(i).translation().z() ==
              gm(magnet.zStart + magnet.halfZ - MuonShieldFactory::s_centreZ));

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

TEST_CASE("MuonShieldSndHoleInM5AndM6", "[muonshield]") {
    for (const auto& magnet : MuonShieldFactory::k_magnets) {
        const std::string name = magnet.name;
        INFO("magnet " << name);
        CHECK(MuonShieldFactory::hasSndHole(magnet) == (name == "magn_5" || name == "magn_6"));
    }
}

// M5 holds the veto, target and MGC steps of the cavity, M6 only the HCAL
// step, which spans it face to face.
TEST_CASE("MuonShieldCavityStepsPerMagnet", "[muonshield]") {
    const auto& m5 = MuonShieldFactory::k_magnets.at(4);
    const auto& m6 = MuonShieldFactory::k_magnets.at(5);
    const auto& cavity = MuonShieldFactory::k_sndCavity;
    for (std::size_t i = 0; i < cavity.size(); ++i) {
        INFO("cavity step " << i);
        CHECK(MuonShieldFactory::cuts(cavity.at(i), m5) == (i < 3));
        CHECK(MuonShieldFactory::cuts(cavity.at(i), m6) == (i == 3));
    }
    CHECK(cavity.back().zStart == m6.zStart);
    CHECK(cavity.back().zEnd == m6.zStart + 2 * m6.halfZ);
}
