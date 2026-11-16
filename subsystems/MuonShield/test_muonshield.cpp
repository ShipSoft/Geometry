// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"
#include "SHiP/geometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>

#include <catch2/catch_test_macros.hpp>
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

        // Magnets with the SND hole are a box minus the hole; the others are boxes.
        const GeoShape* shape = child->getLogVol()->getShape();
        if (MuonShieldFactory::hasSndHole(magnet)) {
            const auto* cut = dynamic_cast<const GeoShapeSubtraction*>(shape);
            REQUIRE(cut != nullptr);
            shape = cut->getOpA();
        }
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
