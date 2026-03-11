// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoVFullPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// CSV limits: UpstreamTagger halfX ≤ 2200, halfY ≤ 3200, halfZ ≤ 200
TEST_CASE("UpstreamTaggerWithinEnvelope", "[upstreamtagger]") {
    SHiPMaterials materials;
    SHiPGeometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(ubt->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2200.0);
    CHECK(box->getYHalfLength() <= 3200.0);
    CHECK(box->getZHalfLength() <= 200.0);
}

// UpstreamTagger slab must be a GeoVFullPhysVol (sensitive volume)
TEST_CASE("UBTHasSensitiveVolume", "[upstreamtagger]") {
    SHiPMaterials materials;
    SHiPGeometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    CHECK(dynamic_cast<const GeoVFullPhysVol*>(ubt) != nullptr);
}
