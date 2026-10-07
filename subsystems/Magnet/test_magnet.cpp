// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Magnet/MagnetFactory.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiP::geometry::Materials;

TEST_CASE("MagnetBuilds", "[magnet]") {
    Materials materials;
    SHiP::geometry::MagnetFactory factory(materials);
    GeoPhysVol* magnet = factory.build();
    REQUIRE(magnet != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(magnet->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() == 3250.0);
    CHECK(box->getYHalfLength() == 4300.0);
    CHECK(box->getZHalfLength() == 2500.0);
}
