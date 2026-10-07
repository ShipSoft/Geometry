// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"

#include <catch2/catch_test_macros.hpp>

using SHiP::geometry::Materials;

TEST_CASE("MaterialsTest.MaterialsAvailable", "[materials]") {
    Materials mats;
    CHECK_NOTHROW(mats.requireMaterial("Iron"));
    CHECK_NOTHROW(mats.requireMaterial("Air"));
    CHECK_NOTHROW(mats.requireMaterial("Vacuum"));
}
