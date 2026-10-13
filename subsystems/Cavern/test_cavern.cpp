// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Cavern/CavernFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

using SHiPGeometry::SHiPMaterials;

TEST_CASE("CavernWorldVolume", "[cavern]") {
    SHiPMaterials materials;
    SHiPGeometry::CavernFactory factory(materials);
    GeoPhysVol* world = factory.build();
    REQUIRE(world != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(world->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() == 200000.0);
    CHECK(box->getYHalfLength() == 200000.0);
    CHECK(box->getZHalfLength() == 200000.0);
}

// Probe the rock at points in the global frame (mm, origin at the target
// front face). The reference values are those of the integration layout
// SPSXXSHIP0002 version 2026-0.1 (EDMS 3287817 v1.1), not the factory's
// constants, so the test compares the code with the drawing.
TEST_CASE("CavernCavities", "[cavern]") {
    SHiPMaterials materials;
    SHiPGeometry::CavernFactory factory(materials);
    GeoPhysVol* world = factory.build();
    REQUIRE(world->getNChildVols() == 1);
    const GeoVPhysVol* rock = &*world->getChildVol(0);
    REQUIRE(rock->getLogVol()->getName() == std::string("/SHiP/cavern"));
    const GeoShape* shape = rock->getLogVol()->getShape();
    const GeoTrf::Transform3D toRock = world->getXToChildVol(0).inverse();

    auto isRock = [&](double x, double y, double z) {
        const GeoTrf::Vector3D p = toRock * GeoTrf::Vector3D(x, y, z);
        return shape->contains(p.x(), p.y(), p.z());
    };
    constexpr double eps = 10.0;

    SECTION("TCC8 floor 1.70 m below the beam") {
        CHECK_FALSE(isRock(0, -1700 + eps, 10000));
        CHECK(isRock(0, -1700 - eps, 10000));
    }
    SECTION("TCC8 floor ends at 21.37 m, ECN3 floor starts 0.82 m later") {
        CHECK(isRock(0, -2000, 21370 - eps));
        CHECK_FALSE(isRock(0, -2000, 21370 + eps));
        CHECK(isRock(0, -3000, 22190 - eps));
        CHECK_FALSE(isRock(0, -3000, 22190 + eps));
    }
    SECTION("ECN3 floor 3.36 m below the beam") {
        CHECK_FALSE(isRock(0, -3360 + eps, 50000));
        CHECK(isRock(0, -3360 - eps, 50000));
    }
    SECTION("ECN3 end wall at 120.48 m") {
        CHECK_FALSE(isRock(0, 0, 120480 - eps));
        CHECK(isRock(0, 0, 120480 + eps));
    }
    SECTION("Detector chain from target to calorimeter is in air") {
        CHECK_FALSE(isRock(0, 0, 0));
        CHECK_FALSE(isRock(0, 0, 21780));
        CHECK_FALSE(isRock(0, 0, 99770));  // downstream end of the HCAL envelope
    }
    SECTION("Yoke pit 1 m deep, centred on the magnet at 89.72 m") {
        CHECK_FALSE(isRock(0, -4360 + eps, 89720));
        CHECK(isRock(0, -4360 - eps, 89720));
        CHECK(isRock(0, -3360 - eps, 80000));
    }
    SECTION("Target pit 1 m deep below the TCC8 floor") {
        CHECK_FALSE(isRock(0, -2700 + eps, 1460));
        CHECK(isRock(0, -2700 - eps, 1460));
    }
}
