// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/DecayVolumeFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>

using SHiPGeometry::SHiPMaterials;

// CSV limits: DecayVolume halfX ≤ 2200, halfY ≤ 3200, halfZ ≤ 25200
TEST_CASE("DecayVolumeWithinEnvelope", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(dv->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2200.0);
    CHECK(box->getYHalfLength() <= 3200.0);
    CHECK(box->getZHalfLength() <= 25200.0);
}
