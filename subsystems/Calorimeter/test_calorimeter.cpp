// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using SHiP::geometry::CalorimeterFactory;
using SHiP::geometry::Materials;

TEST_CASE("CalorimeterBuilds", "[calorimeter]") {
    Materials materials;
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
    Materials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    // One volume per module per non-air-gap layer, so the sequencer cannot
    // silently drop or duplicate a placement.
    constexpr unsigned kExpectedChildren = SHiP::geometry::Calo::kModuleNX *
                                           SHiP::geometry::Calo::kModuleNY *
                                           SHiP::geometry::Calo::kVolsPerModule;
    CHECK(calo->getNChildVols() == kExpectedChildren);  // NOLINT(readability/check)
}

TEST_CASE("TotalStackZMatchesReference", "[calorimeter]") {
    // Pinned reference: 40 lead + 40 scint + 8 HPL + 1 air gap in the ECAL
    // (1600 mm), 100 mm gap, 5 iron + 5 scint in the HCAL (900 mm).
    // Guards the layer-sequence transcription against accidental edits.
    CHECK_THAT(SHiP::geometry::Calo::kTotalStackZ, Catch::Matchers::WithinAbs(2600.0, 1e-9));
}
