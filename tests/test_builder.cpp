// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/DetectorBuilder.h"

#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiP::geometry::DetectorBuilder;

TEST_CASE("BuilderTest.BuilderReturnsNonNull", "[builder]") {
    DetectorBuilder builder;
    GeoPhysVol* world = builder.build();
    CHECK(world != nullptr);
}

TEST_CASE("BuilderTest.WorldHasChildren", "[builder]") {
    DetectorBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);
    CHECK(world->getNChildVols() >= 1u);  // NOLINT(readability/check)
}
