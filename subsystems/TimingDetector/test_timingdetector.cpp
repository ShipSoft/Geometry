// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "TimingDetector/TimingDetectorFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// CSV limits: TimingDetector halfX ≤ 2750, halfY ≤ 3250, halfZ ≤ 250
TEST_CASE("TimingDetectorWithinEnvelope", "[timingdetector]") {
    SHiPMaterials materials;
    SHiPGeometry::TimingDetectorFactory factory(materials);
    GeoPhysVol* td = factory.build();
    REQUIRE(td != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(td->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2750.0);
    CHECK(box->getYHalfLength() <= 3250.0);
    CHECK(box->getZHalfLength() <= 250.0);
}

// GeoModelXML prototype: 3 columns × 110 bars = 330 sensitive volumes
TEST_CASE("TimingDetectorBarCount", "[timingdetector]") {
    SHiPMaterials materials;
    SHiPGeometry::TimingDetectorFactory factory(materials);
    GeoPhysVol* td = factory.build();
    REQUIRE(td != nullptr);
    CHECK(factory.barCount() == 330);  // NOLINT(readability/check)
}
