// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldConfig.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
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
    // A block that fits unrotated but whose rotated extent exceeds the envelope
    // is rejected: the parser and the factory both work from the true corners.
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

namespace {
// An Air box to embed, with the given half-sizes.
GeoPhysVol* makeDummy(SHiPMaterials& materials, double hx, double hy, double hz) {
    auto* box = new GeoBox(hx, hy, hz);
    auto* log = new GeoLogVol("/SHiP/dummy", box, materials.requireMaterial("Air"));
    return new GeoPhysVol(log);
}
}  // namespace

TEST_CASE("MuonShieldEmbedsDaughter", "[muonshield]") {
    SHiPMaterials materials;
    GeoPhysVol* dPhys = makeDummy(materials, 100.0, 100.0, 500.0);

    MuonShieldFactory factory(materials);  // default 7 solid magnets
    factory.reserveSpace({0.0, 0.0, 28950.0}, {400.0, 400.0, 1200.0});
    factory.embedDaughter(dPhys, {0.0, 0.0, 28950.0}, {0.0, 0.0, 0.0}, "/SHiP/dummy");
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

TEST_CASE("MuonShieldPlacesDaughterOffAxis", "[muonshield]") {
    // embedDaughter takes the full centre, so an off-axis daughter is placed
    // off-axis — and at the same place its cavity was carved.
    SHiPMaterials materials;
    GeoPhysVol* dPhys = makeDummy(materials, 100.0, 100.0, 500.0);

    MuonShieldFactory factory(materials);
    factory.reserveSpace({300.0, -50.0, 28950.0}, {400.0, 400.0, 1200.0});
    factory.embedDaughter(dPhys, {300.0, -50.0, 28950.0}, {0.0, 0.0, 0.0}, "/SHiP/dummy");
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);

    const unsigned last = ms->getNChildVols() - 1;
    REQUIRE(ms->getChildVol(last)->getLogVol()->getName() == "/SHiP/dummy");
    // getXToChildVol returns by value, so the transform must outlive the read.
    const GeoTrf::Transform3D toDaughter = ms->getXToChildVol(last);
    const auto translation = toDaughter.translation();
    CHECK_THAT(translation.x(), Catch::Matchers::WithinAbs(300.0, 1e-6));
    CHECK_THAT(translation.y(), Catch::Matchers::WithinAbs(-50.0, 1e-6));
    CHECK_THAT(translation.z(), Catch::Matchers::WithinAbs(28950.0 - 18310.0, 1e-6));
}

TEST_CASE("MuonShieldRejectsDaughterOverrunningEnvelope", "[muonshield]") {
    // The containment check uses the daughter's own extent, not just its
    // centre: this centre is inside the envelope (|31500 - 18310| < 13770) but
    // the 5100 mm-long box reaches 1970 mm past the downstream end.
    SHiPMaterials materials;
    GeoPhysVol* dPhys = makeDummy(materials, 400.0, 400.0, 2550.0);

    MuonShieldFactory factory(materials);
    factory.reserveSpace({0.0, 0.0, 31500.0}, {900.0, 900.0, 5200.0});
    factory.embedDaughter(dPhys, {0.0, 0.0, 31500.0}, {0.0, 0.0, 0.0}, "/SHiP/dummy");
    CHECK_THROWS_AS(factory.build(), std::runtime_error);
}

TEST_CASE("MuonShieldRejectsDaughterOutsideCavity", "[muonshield]") {
    // A daughter with no reservation covering it would sit in solid iron.
    SHiPMaterials materials;
    MuonShieldFactory noReservation(materials);
    noReservation.embedDaughter(makeDummy(materials, 100.0, 100.0, 500.0), {0.0, 0.0, 28950.0},
                                {0.0, 0.0, 0.0}, "/SHiP/dummy");
    CHECK_THROWS_AS(noReservation.build(), std::runtime_error);

    // Likewise when the daughter outgrows the cavity declared for it — the
    // drift that would otherwise go unnoticed between SD.toml and the SND
    // factory's own container dimensions.
    MuonShieldFactory tooSmall(materials);
    tooSmall.reserveSpace({0.0, 0.0, 28950.0}, {400.0, 400.0, 1200.0});
    tooSmall.embedDaughter(makeDummy(materials, 100.0, 100.0, 900.0), {0.0, 0.0, 28950.0},
                           {0.0, 0.0, 0.0}, "/SHiP/dummy");
    CHECK_THROWS_AS(tooSmall.build(), std::runtime_error);
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

TEST_CASE("MuonShieldRejectsRotatedOverlappingBlocks", "[muonshield]") {
    // Two slabs rotated 90° about Z that genuinely intersect. Their unrotated
    // bounding boxes are disjoint in X, so an axis-aligned test would accept
    // them; the separating-axis test on the oriented boxes does not.
    const std::string path = writeTempToml(
        "MS_rot_overlap.toml",
        "envelope_half_x_mm = 3000\nenvelope_half_y_mm = 3000\n"
        "envelope_z_start_m = 0.0\nenvelope_z_end_m = 6.0\n"
        "[[block]]\nstart = [-1000,0,2000]\nsize = [200,4000,400]\nrotation = [0,0,90]\n"
        "[[block]]\nstart = [1000,0,2000]\nsize = [200,4000,400]\nrotation = [0,0,90]\n");
    CHECK_THROWS_AS(readMuonShieldConfig(path), std::runtime_error);
}

TEST_CASE("MuonShieldAcceptsRotatedBlockThatFits", "[muonshield]") {
    // The inverse of MuonShieldRejectsRotatedBlockOutsideEnvelope: rotating a
    // slab by 90° about Z swaps its X and Y extents, and here that makes it fit
    // an envelope its unrotated footprint would overflow.
    const std::string path = writeTempToml(
        "MS_rot_fits.toml",
        "envelope_half_x_mm = 200\nenvelope_half_y_mm = 1500\n"
        "envelope_z_start_m = 0.0\nenvelope_z_end_m = 6.0\n"
        "[[block]]\nstart = [0,0,2000]\nsize = [2400,200,400]\nrotation = [0,0,90]\n");
    const MuonShieldConfig cfg = readMuonShieldConfig(path);
    CHECK(cfg.blocks.size() == 1u);  // NOLINT(readability/check)

    SHiPMaterials materials;
    MuonShieldFactory factory(materials, path);
    CHECK(factory.build() != nullptr);
}

TEST_CASE("MuonShieldAllowsTouchingBlocks", "[muonshield]") {
    // Blocks sharing a face are legal; only interpenetration is rejected.
    const std::string path =
        writeTempToml("MS_touching.toml",
                      "envelope_half_x_mm = 2000\nenvelope_half_y_mm = 2000\n"
                      "envelope_z_start_m = 0.0\nenvelope_z_end_m = 6.0\n"
                      "[[block]]\nstart = [0,0,1000]\nsize = [1000,1000,1000]\n"
                      "[[block]]\nstart = [0,0,2000]\nsize = [1000,1000,1000]\n");
    const MuonShieldConfig cfg = readMuonShieldConfig(path);
    CHECK(cfg.blocks.size() == 2u);  // NOLINT(readability/check)
}

TEST_CASE("MuonShieldFailedBuildLeavesFactoryUsable", "[muonshield]") {
    // A build() that throws must not latch m_built: the factory stays usable,
    // and centreZ_mm() reports that it has no value rather than returning 0.0
    // (which would place the container at the world origin).
    const std::string path =
        writeTempToml("MS_bad.toml", "[[block]]\nstart = [0,0,12000]\nsize = [-1,2000,2000]\n");
    SHiPMaterials materials;
    MuonShieldFactory factory(materials, path);
    CHECK_THROWS_AS(factory.build(), std::runtime_error);
    CHECK_THROWS_AS(factory.centreZ_mm(), std::runtime_error);

    // A fresh factory on a good config still works, and only then does
    // centreZ_mm() answer.
    MuonShieldFactory good(materials);
    CHECK_THROWS_AS(good.centreZ_mm(), std::runtime_error);
    REQUIRE(good.build() != nullptr);
    CHECK_THAT(good.centreZ_mm(), Catch::Matchers::WithinAbs(18310.0, 1e-6));
}
