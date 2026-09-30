// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"
#include "NeutrinoDetector/SNDEnvelope.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;
using SHiPGeometry::SHiPMaterials;

// CSV limits: SND half-width/height ≤ 400 mm, length 5100 mm (box approximation).
TEST_CASE("NeutrinoDetectorWithinEnvelope", "[neutrinodetector]") {
    SHiPMaterials materials;
    SHiPGeometry::NeutrinoDetectorFactory factory(materials);
    GeoPhysVol* snd = factory.build();
    REQUIRE(snd != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(snd->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 400.0);
    CHECK(box->getYHalfLength() <= 400.0);
    CHECK(box->getZHalfLength() <= 2550.0);
}

// The container is sized from the SD.toml envelope, the same box that is carved
// out of the muon shield, so the two cannot drift apart.
TEST_CASE("NeutrinoDetectorContainerMatchesEnvelope", "[neutrinodetector]") {
    SHiPMaterials materials;
    const SHiPGeometry::SNDEnvelope env = SHiPGeometry::readSNDEnvelope();
    SHiPGeometry::NeutrinoDetectorFactory factory(materials, env);
    const GeoPhysVol* snd = factory.build();
    REQUIRE(snd != nullptr);
    const auto* box = dynamic_cast<const GeoBox*>(snd->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() == 0.5 * env.size_mm[0]);
    CHECK(box->getYHalfLength() == 0.5 * env.size_mm[1]);
    CHECK(box->getZHalfLength() == 0.5 * env.size_mm[2]);
}

// A custom envelope size is honoured, and one too small for the contents is rejected.
TEST_CASE("NeutrinoDetectorContainerFromCustomEnvelope", "[neutrinodetector]") {
    SHiPMaterials materials;
    SHiPGeometry::SNDEnvelope env;
    env.size_mm = {700.0, 650.0, 4000.0};
    SHiPGeometry::NeutrinoDetectorFactory factory(materials, env);
    const GeoPhysVol* snd = factory.build();
    const auto* box = dynamic_cast<const GeoBox*>(snd->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() == 350.0);
    CHECK(box->getYHalfLength() == 325.0);
    CHECK(box->getZHalfLength() == 2000.0);

    SHiPGeometry::SNDEnvelope tooNarrow;
    tooNarrow.size_mm = {500.0, 800.0, 5100.0};  // HCAL needs ~600.5 mm
    SHiPGeometry::NeutrinoDetectorFactory narrowFactory(materials, tooNarrow);
    CHECK_THROWS_WITH(narrowFactory.build(), ContainsSubstring("too small"));

    SHiPGeometry::SNDEnvelope tooShort;
    tooShort.size_mm = {800.0, 800.0, 3000.0};  // contents are 3988 mm long
    SHiPGeometry::NeutrinoDetectorFactory shortFactory(materials, tooShort);
    CHECK_THROWS_WITH(shortFactory.build(), ContainsSubstring("too small"));
}

// The container holds the veto, target and HCAL children directly. Counts:
//   veto    3 planes × 7 bars                              =   21
//   target  120 layers × (W + Si-X + Si-Y)                 =  360
//   HCAL    Σ_section 14 × (Fe + FibreX + FibreY + tiles)
//             S0 14×(3+8²)=938, S1 14×(3+10²)=1442, S2 14×(3+12²)=2058 = 4438
//                                                            ------
//                                                             4819
TEST_CASE("NeutrinoDetectorChildCount", "[neutrinodetector]") {
    SHiPMaterials materials;
    SHiPGeometry::NeutrinoDetectorFactory factory(materials);
    GeoPhysVol* snd = factory.build();
    REQUIRE(snd != nullptr);
    CHECK(snd->getNChildVols() == 4819u);  // NOLINT(readability/check)
}
