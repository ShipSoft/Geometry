// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "UpstreamTagger/SHiPUBTManager.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoVFullPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;
using SHiPGeometry::SHiPUBTManager;
using SHiPGeometry::UpstreamTaggerFactory;

// CSV row: Upstream background tagger, half_width=0.75m=750mm, half_height=1.60m=1600mm
TEST_CASE("UBTEnvelopeWithinCSVLimits", "[upstreamtagger]") {
    SHiPMaterials materials;
    UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);

    auto* box = dynamic_cast<const GeoBox*>(ubt->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 750.0);
    CHECK(box->getYHalfLength() <= 1600.0);
    CHECK(box->getZHalfLength() <= 200.0);
}

TEST_CASE("UBTMaterialsExist", "[upstreamtagger]") {
    SHiPMaterials materials;
    CHECK(materials.getMaterial("Mylar")       != nullptr);
    CHECK(materials.getMaterial("ArCO2")       != nullptr);
    CHECK(materials.getMaterial("Polystyrene") != nullptr);
}

TEST_CASE("UBTManagerReceivesSensitiveVolumes", "[upstreamtagger]") {
    SHiPMaterials materials;
    UpstreamTaggerFactory factory(materials);
    SHiPUBTManager manager;
    factory.build(&manager);

    // 2 tile blocks × 15 tiles in X × 40 tiles in Y = 1200
    CHECK(manager.numTubeGasVolumes() > 0);
    CHECK(manager.numTileVolumes() == 1200);
    CHECK(manager.getNumTreeTops() == manager.numTubeGasVolumes() + manager.numTileVolumes());
}

TEST_CASE("UBTBuildWithoutManager", "[upstreamtagger]") {
    SHiPMaterials materials;
    UpstreamTaggerFactory factory(materials);
    CHECK(factory.build(nullptr) != nullptr);
}
