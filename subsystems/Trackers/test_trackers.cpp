// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "Trackers/TrackersFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

using SHiPGeometry::SHiPMaterials;

static const GeoVPhysVol* findChild(const GeoVPhysVol* parent, const std::string& name) {
    for (unsigned int i = 0; i < parent->getNChildVols(); ++i) {
        PVConstLink child = parent->getChildVol(i);
        if (child->getLogVol()->getName() == name) {
            return &*child;
        }
    }
    return nullptr;
}

// CSV limits: Trackers per-station halfX ≤ 3000, halfY ≤ 3500, halfZ ≤ 500
TEST_CASE("TrackersWithinEnvelope", "[trackers]") {
    SHiPMaterials materials;
    SHiPGeometry::TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* st1 = findChild(tc, "TrackerStation_1");
    INFO("TrackerStation_1 not found");
    REQUIRE(st1 != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(st1->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 3000.0);
    CHECK(box->getYHalfLength() <= 3500.0);
    CHECK(box->getZHalfLength() <= 500.0);
}
