// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// CSV limits: SND half-width/height ≤ 0.40 m, length 5.10 m (box approximation).
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
