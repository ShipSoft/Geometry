// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "Target/TargetFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/Units.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using Catch::Approx;
using SHiPGeometry::SHiPMaterials;

namespace {
constexpr double mm = GeoModelKernelUnits::mm;
// Target frame: z = 0 at the front face of the first disk; the HeVolume tube
// is centred on the middle of its target-frame span [-35.8, 1509.7] mm
constexpr double heCentreZ = 0.5 * (-35.8 * mm + 1509.7 * mm);
}  // namespace

TEST_CASE("TargetBuilds", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* target = factory.build();
    REQUIRE(target != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(target->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Vacuum box: 80×113.55×150 cm (half-sizes)
    CHECK(box->getXHalfLength() == 800.0);
    CHECK(box->getYHalfLength() == 1135.5);
    CHECK(box->getZHalfLength() == 1500.0);
}

TEST_CASE("Target2026HeVolume", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* target = factory.build();
    REQUIRE(target != nullptr);

    // Children: proximity, top, bottom, pedestal, he_volume
    REQUIRE(target->getNChildVols() == 5u);
    PVConstLink heVolume = target->getChildVol(4);
    REQUIRE(heVolume->getLogVol()->getName() == "/SHiP/target/he_volume");

    auto* heTube = dynamic_cast<const GeoTube*>(heVolume->getLogVol()->getShape());
    REQUIRE(heTube != nullptr);
    CHECK(heTube->getRMax() == Approx(237.0 * mm));
    CHECK(heTube->getZHalfLength() == Approx(0.5 * (1509.7 * mm + 35.8 * mm)));

    // 33 disks + steel core + jacket + 2 flanges + endcap
    REQUIRE(heVolume->getNChildVols() == 38u);
}

TEST_CASE("Target2026Disks", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* target = factory.build();
    PVConstLink heVolume = target->getChildVol(4);

    // Disk 1: 30 mm thick, front face at target z = 0
    {
        PVConstLink disk = heVolume->getChildVol(0);
        auto* tube = dynamic_cast<const GeoTube*>(disk->getLogVol()->getShape());
        REQUIRE(tube != nullptr);
        CHECK(tube->getRMax() == Approx(125.0 * mm));
        CHECK(tube->getZHalfLength() == Approx(15.0 * mm));
        const double zCentre = heVolume->getXToChildVol(0).translation().z() + heCentreZ;
        CHECK(zCentre - tube->getZHalfLength() == Approx(0.0).margin(1e-9));
    }

    // Disk 33 (rear block): larger radius, spans 1005-1460 mm
    {
        PVConstLink disk = heVolume->getChildVol(32);
        auto* tube = dynamic_cast<const GeoTube*>(disk->getLogVol()->getShape());
        REQUIRE(tube != nullptr);
        CHECK(tube->getRMax() == Approx(157.0 * mm));
        const double zCentre = heVolume->getXToChildVol(32).translation().z() + heCentreZ;
        CHECK(zCentre - tube->getZHalfLength() == Approx(1005.0 * mm));
        CHECK(zCentre + tube->getZHalfLength() == Approx(1460.0 * mm));
    }

    // Total tungsten volume: 875 mm of W at r=125 plus the 455 mm rear block
    // at r=157
    double totalVolume = 0.0;
    for (unsigned int i = 0; i < 33; ++i) {
        auto* tube =
            dynamic_cast<const GeoTube*>(heVolume->getChildVol(i)->getLogVol()->getShape());
        REQUIRE(tube != nullptr);
        totalVolume +=
            std::numbers::pi * tube->getRMax() * tube->getRMax() * 2.0 * tube->getZHalfLength();
    }
    const double expected = std::numbers::pi * (125.0 * mm) * (125.0 * mm) * 875.0 * mm +
                            std::numbers::pi * (157.0 * mm) * (157.0 * mm) * 455.0 * mm;
    CHECK(totalVolume == Approx(expected));
}

TEST_CASE("Target2026SteelCore", "[target]") {
    SHiPMaterials materials;
    SHiPGeometry::TargetFactory factory(materials);
    GeoPhysVol* target = factory.build();
    PVConstLink heVolume = target->getChildVol(4);

    // Child 33 is the steel core: a polycone with the He cooling grooves
    // subtracted
    PVConstLink core = heVolume->getChildVol(33);
    CHECK(core->getLogVol()->getName() == "/SHiP/target/core_steel");   // NOLINT(readability/check)
    CHECK(core->getLogVol()->getMaterial()->getName() == "Steel316L");  // NOLINT(readability/check)
    auto* subtraction = dynamic_cast<const GeoShapeSubtraction*>(core->getLogVol()->getShape());
    REQUIRE(subtraction != nullptr);
}
