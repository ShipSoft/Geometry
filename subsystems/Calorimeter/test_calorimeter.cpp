// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterConstants.h"
#include "Calorimeter/CalorimeterFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using SHiPGeometry::CalorimeterFactory;
using SHiPGeometry::SHiPMaterials;

TEST_CASE("CalorimeterBuilds", "[calorimeter]") {
    SHiPMaterials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(calo->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Fixed envelope: 3.00 × 3.50 × 1.45 m half-sizes
    CHECK(box->getXHalfLength() == 3000.0);
    CHECK(box->getYHalfLength() == 3500.0);
    CHECK(box->getZHalfLength() == 1450.0);
}

TEST_CASE("CalorimeterHasChildren", "[calorimeter]") {
    SHiPMaterials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    // The container must have at least one child (ECAL layers + HCAL layers)
    CHECK(calo->getNChildVols() >= 1u);  // NOLINT(readability/check)
}

TEST_CASE("TotalStackZMatchesReference", "[calorimeter]") {
    // Pinned reference: 40 lead + 40 scint + 8 HPL + 1 air gap in the ECAL
    // (1600 mm), 100 mm gap, 5 iron + 5 scint in the HCAL (900 mm).
    // Guards the layer-sequence transcription against accidental edits.
    CHECK_THAT(SHiPGeometry::Calo::kTotalStackZ, Catch::Matchers::WithinAbs(2600.0, 1e-9));
}
