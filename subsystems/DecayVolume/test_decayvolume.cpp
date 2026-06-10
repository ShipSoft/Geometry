// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/DecayVolumeFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

using SHiPGeometry::SHiPMaterials;

namespace {
// The SBT is placed flat (every child of the container is a leaf), so the
// shapes of the direct children fully describe the geometry: GeoBox = steel
// structure, GeoTrap = helium + sensor walls/cells.
struct ChildShapeCounts {
    unsigned total = 0;
    unsigned boxes = 0;
    unsigned traps = 0;
    unsigned helium = 0;
};

ChildShapeCounts countByShape(const GeoVPhysVol* vol) {
    ChildShapeCounts c;
    c.total = vol->getNChildVols();
    for (unsigned int i = 0; i < vol->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*vol->getChildVol(i);
        const GeoLogVol* lv = child->getLogVol();
        const GeoShape* shape = lv->getShape();
        if (dynamic_cast<const GeoBox*>(shape))
            ++c.boxes;
        if (dynamic_cast<const GeoTrap*>(shape))
            ++c.traps;
        if (lv->getName().find("helium") != std::string::npos)
            ++c.helium;
    }
    return c;
}
}  // namespace

// The container is an air box enclosing the SBT structure + sensors and the
// central helium frustum.
// CSV limits: DecayVolume halfX <= 2200, halfY <= 3300, halfZ <= 25200
TEST_CASE("DecayVolumeWithinEnvelope", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(dv->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2200.0);
    CHECK(box->getYHalfLength() <= 3300.0);
    CHECK(box->getZHalfLength() <= 25200.0);
}

// Steel H-beam structure: 66 column + 120 corner-beam + 60 longitudinal +
// 66 cross-beam GeoBox pieces = 312, all direct children of the container.
TEST_CASE("DecayVolumeStructureBoxCount", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const ChildShapeCounts c = countByShape(dv);
    CHECK(c.boxes == 312u);  // NOLINT(readability/check)
}

// Sensor system: 130 containers, each Z-split into 2 pieces of 7 Al walls +
// 6 LAB cells (13 GeoTraps) -> 130*2*13 = 3380 sensor traps; plus the single
// helium frustum = 3381 GeoTrap children.
TEST_CASE("DecayVolumeSensorTrapCount", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const ChildShapeCounts c = countByShape(dv);
    CHECK(c.traps == 3381u);  // NOLINT(readability/check)
    CHECK(c.helium == 1u);    // NOLINT(readability/check)
}

// Flat architecture: total direct children = 312 structure + 3380 sensors +
// 1 helium = 3693, with no grandchildren.
TEST_CASE("DecayVolumeChildCount", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    CHECK(dv->getNChildVols() == 3693u);  // NOLINT(readability/check)
}

// The central decay region is a helium GeoTrap (frustum), sized inside the
// innermost sensor faces so it cannot overlap the structure or sensors.
TEST_CASE("DecayVolumeHasHeliumFrustum", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const GeoVPhysVol* he = nullptr;
    for (unsigned int i = 0; i < dv->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*dv->getChildVol(i);
        if (child->getLogVol()->getName().find("helium") != std::string::npos) {
            he = child;
            break;
        }
    }
    REQUIRE(he != nullptr);
    auto* trap = dynamic_cast<const GeoTrap*>(he->getLogVol()->getShape());
    REQUIRE(trap != nullptr);
}
