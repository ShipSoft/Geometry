// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// MuonShieldArea container halfX ≤ 2100 (CSV WARM max half-width),
// halfY ≤ 2300 (CSV WARM max half-height)
TEST_CASE("MuonShieldWithinEnvelope", "[muonshield]") {
    SHiPMaterials materials;
    SHiPGeometry::MuonShieldFactory factory(materials);
    GeoPhysVol* ms = factory.build();
    REQUIRE(ms != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(ms->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2100.0);
    CHECK(box->getYHalfLength() <= 2300.0);
}
