// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"
#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

using SHiP::geometry::CalorimeterFactory;
using SHiP::geometry::Materials;

TEST_CASE("CalorimeterBuilds", "[calorimeter]") {
    Materials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    const auto* box = dynamic_cast<const GeoBox*>(calo->getLogVol()->getShape());
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
    constexpr unsigned kExpectedChildren = SHiP::geometry::calo::kModuleNX *
                                           SHiP::geometry::calo::kModuleNY *
                                           SHiP::geometry::calo::kVolsPerModule;
    CHECK(calo->getNChildVols() == kExpectedChildren);  // NOLINT(readability/check)
}

// ── HPL fibre layout ─────────────────────────────────────────────────────────

namespace {

namespace calo = SHiP::geometry::calo;
using SHiP::geometry::units::gm;

/// Distance between the centres of two fibres, in mm.
double fibreDistance(int s1, int i1, int s2, int i2) {
    const auto a = calo::fibreCentre(s1, i1);
    const auto b = calo::fibreCentre(s2, i2);
    return std::hypot(gm(a.across) - gm(b.across), gm(a.z) - gm(b.z));
}

}  // namespace

TEST_CASE("HplFibreCountsPerSublayer", "[calorimeter]") {
    CHECK(calo::fibresPerSublayer(0) == 1799);
    CHECK(calo::fibresPerSublayer(1) == 1800);
    CHECK(calo::fibresPerSublayer(2) == 1799);
    CHECK(calo::kFibresPerHpl == 5398);
}

TEST_CASE("HplFibresCloseTouchWithoutOverlap", "[calorimeter]") {
    const double diameter = gm(calo::kFiberDiameter);
    // Each outer fibre j sits between middle fibres j and j + 1; neighbours
    // within a row are one pitch (one diameter) apart. Track the extremes so
    // a single wrong centre shows up without thousands of assertions.
    double minDist = std::numeric_limits<double>::max();
    double maxDist = 0.0;
    const auto record = [&](double d) {
        minDist = std::min(minDist, d);
        maxDist = std::max(maxDist, d);
    };
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        for (int i = 0; i + 1 < calo::fibresPerSublayer(s); ++i) {
            record(fibreDistance(s, i, s, i + 1));
        }
    }
    for (const int outer : {0, 2}) {
        for (int j = 0; j < calo::fibresPerSublayer(outer); ++j) {
            record(fibreDistance(outer, j, 1, j));
            record(fibreDistance(outer, j, 1, j + 1));
        }
    }
    CHECK(minDist == Catch::Approx(diameter));
    CHECK(maxDist == Catch::Approx(diameter));
    // Outer rows are two row spacings apart, well clear of each other.
    CHECK(fibreDistance(0, 0, 2, 0) > diameter);
}

TEST_CASE("HplFibresInsideCasing", "[calorimeter]") {
    const double r = 0.5 * gm(calo::kFiberDiameter);
    const double halfXY = 0.5 * gm(calo::kPlateXY);
    const double halfZ = 0.5 * gm(calo::kHplThickness);
    constexpr double kTol = 1e-9;  // the outermost middle fibres are flush with the casing
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        const int n = calo::fibresPerSublayer(s);
        for (const int i : {0, n - 1}) {
            const auto c = calo::fibreCentre(s, i);
            CHECK(std::abs(gm(c.across)) + r <= halfXY + kTol);
            CHECK(std::abs(gm(c.z)) + r <= halfZ);
        }
    }
}

TEST_CASE("HplBuilderPlacesSharedCentres", "[calorimeter]") {
    Materials materials;
    const GeoMaterial* al = materials.requireMaterial("Aluminium");
    const GeoMaterial* ps = materials.requireMaterial("Polystyrene");
    const GeoMaterial* air = materials.requireMaterial("Air");
    const double half = gm(calo::kPlateXY);
    auto* mother = new GeoPhysVol(new GeoLogVol("mother", new GeoBox(half, half, half), air));
    mother->ref();

    SHiP::geometry::calo_fibre_hp::buildLayer(
        mother, al, ps, "T", 0.0 * SHiP::geometry::units::mm, 0, calo::kPlateXY,
        calo::kHplThickness, calo::kFiberDiameter, calo::kFiberCoreDiameter, true, "");
    REQUIRE(mother->getNChildVols() == 1);
    const PVConstLink casing = mother->getChildVol(0);
    REQUIRE(casing->getNChildVols() == static_cast<unsigned>(calo::kFibresPerHpl));

    // Children are placed sublayer by sublayer, fibre by fibre (fibres along Y,
    // so the across-layer coordinate is X).
    unsigned child = 0;
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        for (int i = 0; i < calo::fibresPerSublayer(s); ++i, ++child) {
            const auto c = calo::fibreCentre(s, i);
            // A concrete Vector3D, not auto: translation() is a view into the
            // temporary transform.
            const GeoTrf::Vector3D t = casing->getXToChildVol(child).translation();
            if (t.x() != gm(c.across) || t.y() != 0.0 || t.z() != gm(c.z)) {
                FAIL("fibre S" << s << "_F" << i << " at (" << t.x() << ", " << t.y() << ", "
                               << t.z() << ")");
            }
        }
    }

    mother->unref();
}

TEST_CASE("HplBuilderRejectsThinCasing", "[calorimeter]") {
    Materials materials;
    const GeoMaterial* al = materials.requireMaterial("Aluminium");
    const GeoMaterial* ps = materials.requireMaterial("Polystyrene");
    const GeoMaterial* air = materials.requireMaterial("Air");
    const double half = gm(calo::kPlateXY);
    auto* mother = new GeoPhysVol(new GeoLogVol("mother", new GeoBox(half, half, half), air));
    mother->ref();
    // 3 mm < 1.2 mm * (1 + sqrt(3)) ≈ 3.28 mm
    CHECK_THROWS_AS(SHiP::geometry::calo_fibre_hp::buildLayer(
                        mother, al, ps, "T", 0.0 * SHiP::geometry::units::mm, 0, calo::kPlateXY,
                        3.0 * SHiP::geometry::units::mm, calo::kFiberDiameter,
                        calo::kFiberCoreDiameter, true, ""),
                    std::runtime_error);
    mother->unref();
}
