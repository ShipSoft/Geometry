// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "Target/TargetFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

TEST_CASE("TargetBuilds", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* target = factory.build();
    REQUIRE(target != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(target->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Vacuum box: 80×113.55×150 cm (half-sizes)
    CHECK(box->getXHalfLength() == 800.0);
    CHECK(box->getYHalfLength() == 1135.5);
    CHECK(box->getZHalfLength() == 1500.0);
}
