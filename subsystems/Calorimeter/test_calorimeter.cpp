// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterConfig.h"
#include "Calorimeter/CalorimeterFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using SHiPGeometry::CalorimeterConfig;
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

TEST_CASE("TotalStackZPositive", "[calorimeter]") {
    // Minimal config exercising all layer types
    CalorimeterConfig cfg;
    cfg.layers = {7, 1, 7, 2};  // lead, PVT-H, lead, PVT-V
    cfg.layers2 = {7, 1};       // iron, PVT-H
    cfg.lead_thickness_mm = 3.0;
    cfg.scint_thickness_mm = 10.0;
    cfg.iron_thickness_mm = 10.0;
    cfg.hpl_thickness_mm = 10.0;
    cfg.fiber_diameter_mm = 1.2;
    cfg.fiber_core_diameter_mm = 1.0;
    cfg.gap_ecal_hcal_mm = 0.0;
    // layers: 2×lead + 2×scint = 6+20 = 26 mm
    // layers2: 1×iron + 1×scint = 10+10 = 20 mm
    // total = 46 mm
    CHECK_THAT(CalorimeterFactory::totalStackZ(cfg), Catch::Matchers::WithinAbs(46.0, 1e-9));
}
