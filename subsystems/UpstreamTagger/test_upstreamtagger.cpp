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

// UpstreamTagger container must be a GeoVFullPhysVol (sensitive tree-top)
TEST_CASE("UBTHasSensitiveVolume", "[upstreamtagger]") {
    SHiPMaterials materials;
    SHiPGeometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    CHECK(dynamic_cast<const GeoVFullPhysVol*>(ubt) != nullptr);
}

// The tile plane is built from seven abutting tile regions.
TEST_CASE("UBTHasSevenRegions", "[upstreamtagger]") {
    SHiPMaterials materials;
    SHiPGeometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    CHECK(ubt->getNChildVols() == 7u);  // NOLINT(readability/check)
}

// Every region is fully tiled; the totals are fixed by the region geometry:
//   fine blocks  2 × (20×20)      =   800
//   coarse central  30×10          =   300
//   coarse bands 2 × (50×32)       =  3200
//   extensions   2 × (40×150)      = 12000
//                                   ------
//                                    16300 tiles
TEST_CASE("UBTTileCount", "[upstreamtagger]") {
    SHiPMaterials materials;
    SHiPGeometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);

    unsigned int totalTiles = 0;
    for (unsigned int i = 0; i < ubt->getNChildVols(); ++i) {
        totalTiles += ubt->getChildVol(i)->getNChildVols();
    }
    CHECK(totalTiles == 16300u);  // NOLINT(readability/check)
}
