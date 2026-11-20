// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiP::geometry::Materials;

// The container holds the veto, target, MGC and HCAL children directly. Counts:
//   veto    3 planes × 7 bars                              =   21
//   target  120 layers × (W + Si-X + Si-Y) + 3 Timing      =  363
//   MGC     10 layers × (W + Si-X + Si-Y)                  =   30
//   HCAL    Σ_section 14 × (Fe + FibreX + FibreY + tiles)
//             S0 14×(3+12²)=2058, S1 14×(3+12²)=2058, S2 14×(3+12²)=2058 = 6174
//                                                            ------
//                                                             6588
TEST_CASE("NeutrinoDetectorChildCount", "[neutrinodetector]") {
    Materials materials;
    SHiP::geometry::NeutrinoDetectorFactory factory(materials);
    GeoPhysVol* snd = factory.build();
    REQUIRE(snd != nullptr);
    CHECK(snd->getNChildVols() == 6588u);  // NOLINT(readability/check)
}
