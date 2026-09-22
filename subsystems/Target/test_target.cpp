// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "Target/TargetFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
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

TEST_CASE("TargetHadronStopper", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* stopper = factory.buildHadronStopper();
    REQUIRE(stopper != nullptr);
    CHECK(stopper->getLogVol()->getName() == "/SHiP/hadron_stopper");  // NOLINT(readability/check)
    CHECK(stopper->getLogVol()->getMaterial()->getName() == "Iron");   // NOLINT(readability/check)

    auto* box = dynamic_cast<const GeoBox*>(stopper->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Outer envelope of the GDML `magn_absorb` pieces: 102.0 x 169.1 x 115.5 cm
    // half-sizes; the 2.31 m length covers the 2.14-4.44 m envelope row.
    CHECK(box->getXHalfLength() == 1020.0);
    CHECK(box->getYHalfLength() == 1691.0);
    CHECK(box->getZHalfLength() == 1155.0);
}
