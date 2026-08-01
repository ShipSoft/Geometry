// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldConfig.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTrd.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

using SHiPGeometry::MuonShieldConfig;
using SHiPGeometry::MuonShieldFactory;
using SHiPGeometry::readMuonShieldConfig;
using SHiPGeometry::SHiPMaterials;

namespace {
// Write a temp muon_shield.toml with the given body and return its path.
std::string writeTempToml(const std::string& name, const std::string& body) {
    std::ofstream out(name);
    out << body;
    out.close();
    return name;
}
}  // namespace

// Default muon_shield.toml: the 7 FairShip TRY_2026 magnets (solid-block approximation)
// inside an auto-sized envelope (4.54–32.08 m, 1760 × 1320 mm half-sizes).
TEST_CASE("MuonShieldBuilds", "[muonshield]") {
    SHiPMaterials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);

    auto* box = dynamic_cast<const GeoBox*>(ms->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK_THAT(box->getXHalfLength(), Catch::Matchers::WithinAbs(1760.0, 1e-6));
    CHECK_THAT(box->getYHalfLength(), Catch::Matchers::WithinAbs(1320.0, 1e-6));
    CHECK_THAT(box->getZHalfLength(), Catch::Matchers::WithinAbs(13770.0, 1e-6));

    // Envelope centre = (4.54 + 32.08)/2 m = 18.31 m.
    CHECK_THAT(factory.centreZ_mm(), Catch::Matchers::WithinAbs(18310.0, 1e-6));
}

TEST_CASE("MuonShieldDefaultLayout", "[muonshield]") {
    SHiPMaterials materials;
    MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    // 7 solid magnets (the SND cavity is carved by reserveSpace, not here).
    REQUIRE(ms->getNChildVols() == 7u);  // NOLINT(readability/check)

    // Magnet 1: straight box, upstream face at z = 4.59 m, 2720 × 1600 × 3000 mm.
    auto* block0 = dynamic_cast<const GeoBox*>(ms->getChildVol(0)->getLogVol()->getShape());
    REQUIRE(block0 != nullptr);
    CHECK_THAT(block0->getXHalfLength(), Catch::Matchers::WithinAbs(1360.0, 1e-6));
    CHECK_THAT(block0->getYHalfLength(), Catch::Matchers::WithinAbs(800.0, 1e-6));
    CHECK_THAT(block0->getZHalfLength(), Catch::Matchers::WithinAbs(1500.0, 1e-6));

    // Anchor (upstream face) at world 4590 → local (4590-18310); centre + halfLen.
    const double blockCentreLocalZ = (4590.0 - 18310.0) + 1500.0;  // = -12220
    CHECK_THAT(ms->getXToChildVol(0).translation().z(),
               Catch::Matchers::WithinAbs(blockCentreLocalZ, 1e-3));

    // Magnet 6 (index 5) is a plain box in the current TRY_2026 config.
    CHECK(dynamic_cast<const GeoBox*>(ms->getChildVol(5)->getLogVol()->getShape()) != nullptr);
}

TEST_CASE("MuonShieldReservationCarvesIron", "[muonshield]") {
    // A reserved box (the SND envelope: 800 × 800 × 5100 mm at z = 28.95 m) is
    // subtracted (A - B) from every magnet it intersects, leaving upstream
    // magnets untouched.
    SHiPMaterials materials;
    MuonShieldFactory factory(materials);
    factory.reserveSpace({0.0, 0.0, 28950.0}, {800.0, 800.0, 5100.0});
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    REQUIRE(ms->getNChildVols() == 7u);

    // Magnet 1 (upstream, far from the SND) is untouched → still a plain box.
    CHECK(dynamic_cast<const GeoBox*>(ms->getChildVol(0)->getLogVol()->getShape()) != nullptr);
    // Both magnets the SND box spans (6 and 7 → indices 5, 6) are carved.
    CHECK(dynamic_cast<const GeoShapeSubtraction*>(ms->getChildVol(5)->getLogVol()->getShape()) !=
          nullptr);
    CHECK(dynamic_cast<const GeoShapeSubtraction*>(ms->getChildVol(6)->getLogVol()->getShape()) !=
          nullptr);
}

TEST_CASE("MuonShieldRejectsRotatedBlockOutsideEnvelope", "[muonshield]") {
    // A block that fits unrotated but whose rotated bounding box exceeds the
    // envelope is rejected at build() (the factory uses the true 8-corner AABB).
    const std::string path = writeTempToml(
        "MS_rot_reject.toml",
        "envelope_half_x_mm = 1500\nenvelope_half_y_mm = 400\n"
        "envelope_z_start_m = 0.0\nenvelope_z_end_m = 6.0\n"
        "[[block]]\nstart = [0,0,2000]\nsize = [2400,200,400]\nrotation = [0,0,90]\n");
    SHiPMaterials materials;
    MuonShieldFactory factory(materials, path);
    CHECK_THROWS_AS(factory.build(), std::runtime_error);
}

TEST_CASE("MuonShieldRotatedReservationCarves", "[muonshield]") {
    // A rotated reservation box still intersects and carves the target magnet.
    SHiPMaterials materials;
    MuonShieldFactory factory(materials);
    factory.reserveSpace({0.0, 0.0, 28950.0}, {800.0, 800.0, 5100.0}, {0.0, 0.0, 45.0});
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    CHECK(dynamic_cast<const GeoShapeSubtraction*>(ms->getChildVol(6)->getLogVol()->getShape()) !=
          nullptr);
}

TEST_CASE("MuonShieldTaperMakesTrd", "[muonshield]") {
    // A block with an X taper becomes a GeoTrd that widens downstream.
    const std::string path = writeTempToml(
        "MS_taper.toml",
        "[[block]]\nstart = [0,0,12000]\nsize = [3000,2000,2000]\ntaper = [5.0, 0.0]\n");
    SHiPMaterials materials;
    MuonShieldFactory factory(materials, path);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    REQUIRE(ms->getNChildVols() == 1u);

    auto* trd = dynamic_cast<const GeoTrd*>(ms->getChildVol(0)->getLogVol()->getShape());
    REQUIRE(trd != nullptr);
    CHECK_THAT(trd->getXHalfLength1(), Catch::Matchers::WithinAbs(1500.0, 1e-6));
    const double farHalfX = 1500.0 + 2000.0 * std::tan(5.0 * 3.14159265358979323846 / 180.0);
    CHECK_THAT(trd->getXHalfLength2(), Catch::Matchers::WithinAbs(farHalfX, 1e-3));
    CHECK_THAT(trd->getYHalfLength1(), Catch::Matchers::WithinAbs(1000.0, 1e-6));
    CHECK_THAT(trd->getYHalfLength2(), Catch::Matchers::WithinAbs(1000.0, 1e-6));
}

TEST_CASE("MuonShieldParsesRotation", "[muonshield]") {
    // The optional `rotation` field is parsed into rotation_deg (degrees, about
    // x, y, z). The factory applies it as an extrinsic X→Y→Z rotation about the
    // block's start; asserting the built orientation would require reading the
    // child transform back, which is left to OverlapCheck / a future test.
    const std::string path = writeTempToml(
        "MS_rot.toml",
        "[[block]]\nstart = [0,0,12000]\nsize = [200,200,400]\nrotation = [10.0, 20.0, 30.0]\n");
    const MuonShieldConfig cfg = readMuonShieldConfig(path);
    REQUIRE(cfg.blocks.size() == 1u);
    CHECK_THAT(cfg.blocks[0].rotation_deg[0], Catch::Matchers::WithinAbs(10.0, 1e-9));
    CHECK_THAT(cfg.blocks[0].rotation_deg[1], Catch::Matchers::WithinAbs(20.0, 1e-9));
    CHECK_THAT(cfg.blocks[0].rotation_deg[2], Catch::Matchers::WithinAbs(30.0, 1e-9));

    // A rotated block still builds and yields a valid shape.
    SHiPMaterials materials;
    MuonShieldFactory factory(materials, path);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    REQUIRE(ms->getNChildVols() == 1u);
    CHECK(dynamic_cast<const GeoBox*>(ms->getChildVol(0)->getLogVol()->getShape()) != nullptr);
}

TEST_CASE("MuonShieldEmbedsDaughter", "[muonshield]") {
    SHiPMaterials materials;
    auto* dBox = new GeoBox(100.0, 100.0, 500.0);
    auto* dLog = new GeoLogVol("/SHiP/dummy", dBox, materials.requireMaterial("Air"));
    auto* dPhys = new GeoPhysVol(dLog);

    MuonShieldFactory factory(materials);  // default 7 solid magnets
    factory.embedDaughter(dPhys, 28.95 * 1000.0, "/SHiP/dummy");
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    // 7 iron magnets + the embedded daughter.
    CHECK(ms->getNChildVols() == 8u);  // NOLINT(readability/check)

    bool found = false;
    for (unsigned i = 0; i < ms->getNChildVols(); ++i)
        if (ms->getChildVol(i)->getLogVol()->getName() == "/SHiP/dummy")
            found = true;
    CHECK(found);
}

TEST_CASE("MuonShieldRejectsNonPositiveSize", "[muonshield]") {
    const std::string path = writeTempToml(
        "MS_badsize.toml", "[[block]]\nstart = [0,0,12000]\nsize = [-3000,2000,2000]\n");
    CHECK_THROWS_AS(readMuonShieldConfig(path), std::runtime_error);
}

TEST_CASE("MuonShieldRejectsCollapsingTaper", "[muonshield]") {
    const std::string path = writeTempToml(
        "MS_badtaper.toml",
        "[[block]]\nstart = [0,0,12000]\nsize = [3000,2000,2000]\ntaper = [-45.0, 0.0]\n");
    CHECK_THROWS_AS(readMuonShieldConfig(path), std::runtime_error);
}

TEST_CASE("MuonShieldRejectsBlockOutsideEnvelope", "[muonshield]") {
    const std::string path = writeTempToml(
        "MS_outside.toml", "[[block]]\nstart = [0,0,40000]\nsize = [3000,2000,1000]\n");
    CHECK_THROWS_AS(readMuonShieldConfig(path), std::runtime_error);
}

TEST_CASE("MuonShieldEmptyBlockList", "[muonshield]") {
    // No blocks → container with no iron (valid; e.g. a placeholder).
    const std::string path = writeTempToml("MS_empty.toml", "block_material = \"Iron\"\n");
    MuonShieldConfig cfg = readMuonShieldConfig(path);
    CHECK(cfg.blocks.empty());
}
