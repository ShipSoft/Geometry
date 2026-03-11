// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Cavern/CavernFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

TEST_CASE("CavernWorldVolume", "[cavern]") {
    SHiPMaterials materials;
    SHiPGeometry::CavernFactory factory(materials);
    GeoPhysVol* world = factory.build();
    REQUIRE(world != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(world->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() == 200000.0);
    CHECK(box->getYHalfLength() == 200000.0);
    CHECK(box->getZHalfLength() == 200000.0);
}
