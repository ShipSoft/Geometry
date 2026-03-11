// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPGeometryBuilder;

TEST_CASE("BuilderTest.BuilderReturnsNonNull", "[builder]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    CHECK(world != nullptr);
}

TEST_CASE("BuilderTest.WorldHasChildren", "[builder]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);
    CHECK(world->getNChildVols() >= 1u);  // NOLINT(readability/check)
}
