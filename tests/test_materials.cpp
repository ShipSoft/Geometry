// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

TEST_CASE("MaterialsTest.MaterialsAvailable", "[materials]") {
    SHiPMaterials mats;
    CHECK_NOTHROW(mats.requireMaterial("Iron"));
    CHECK_NOTHROW(mats.requireMaterial("Air"));
    CHECK_NOTHROW(mats.requireMaterial("Vacuum"));
}
