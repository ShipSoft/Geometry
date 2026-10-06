// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "TimingDetector/TimingDetectorFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// 3 columns × 110 rows = 330 bars, each placed as a child of the container.
TEST_CASE("TimingDetectorBarCount", "[timingdetector]") {
    SHiPMaterials materials;
    SHiPGeometry::TimingDetectorFactory factory(materials);
    GeoPhysVol* td = factory.build();
    REQUIRE(td != nullptr);
    CHECK(factory.barCount() == 330);    // NOLINT(readability/check)
    CHECK(td->getNChildVols() == 330u);  // NOLINT(readability/check)
}
