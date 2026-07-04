// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using SHiPGeometry::SHiPGeometryBuilder;

namespace {

struct SubsystemInfo {
    std::string name;
    double centreZ;  // mm, from world origin
    double halfZ;    // mm
};

// Extract Z translation from a Transform3D
double extractZ(const GeoTrf::Transform3D& trf) {
    return trf.translation().z();
}

// Get the Z half-length of a volume, assuming GeoBox shape
double getHalfZ(const GeoVPhysVol* vol) {
    const auto* box = dynamic_cast<const GeoBox*>(vol->getLogVol()->getShape());
    if (box) {
        return box->getZHalfLength();
    }
    // For non-box shapes (e.g. GeoFullPhysVol with GeoBox), try the tube
    const auto* tube = dynamic_cast<const GeoTube*>(vol->getLogVol()->getShape());
    if (tube) {
        return tube->getZHalfLength();
    }
    return 0.0;
}

// Collect info about all children of the cavern volume (skipping cavern itself)
std::vector<SubsystemInfo> collectSubsystems(GeoPhysVol* world) {
    // The world's first child is the Cavern rock; the subsystems are children
    // of the world placed after the Cavern. Find the cavern first.
    std::vector<SubsystemInfo> subsystems;

    for (unsigned int i = 0; i < world->getNChildVols(); ++i) {
        PVConstLink child = world->getChildVol(i);
        std::string name = child->getLogVol()->getName();
        GeoTrf::Transform3D trf = world->getXToChildVol(i);
        double zCentre = extractZ(trf);
        double halfZ = getHalfZ(&*child);

        // Skip the cavern rock — it's infrastructure, not a subsystem
        if (name == "/SHiP/cavern") {
            continue;
        }

        subsystems.push_back({name, zCentre, halfZ});
    }

    return subsystems;
}

}  // namespace

TEST_CASE("ConsistencyTest.ExpectedSubsystemCount", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);
    // 9 subsystems: target, muon_shield, neutrino_detector, upstream_tagger,
    // decay_volume, trackers, magnet, timing_detector, calorimeter
    CHECK(subsystems.size() == 9u);  // NOLINT(readability/check)
}

TEST_CASE("ConsistencyTest.SubsystemsGenerallyInZOrder", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);
    REQUIRE(subsystems.size() >= 2);

    // Verify monotonically increasing Z for placement order, allowing
    // co-located subsystems (trackers/magnet overlap is intentional).
    // Use a weaker check: each subsystem's centre should be >= previous - tolerance
    for (size_t i = 1; i < subsystems.size(); ++i) {
        INFO("Subsystem " << subsystems[i].name << " at Z=" << subsystems[i].centreZ
                          << " should not be before " << subsystems[i - 1].name
                          << " at Z=" << subsystems[i - 1].centreZ);
        CHECK(subsystems[i].centreZ >= subsystems[i - 1].centreZ - 1.0);
    }
}

TEST_CASE("ConsistencyTest.NoUnexpectedZOverlaps", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);
    REQUIRE(subsystems.size() >= 2);

    // The trackers container intentionally spans across the magnet
    // (stations 1-2 before, stations 3-4 after), so that pair is allowed to overlap.
    // The SND sits inside the downstream end of the muon-shield region, so the
    // muon_shield / neutrino_detector pair is also an intentional overlap.
    auto isAllowedOverlap = [](const std::string& a, const std::string& b) {
        return (a == "/SHiP/trackers" && b == "/SHiP/magnet") ||
               (a == "/SHiP/magnet" && b == "/SHiP/trackers") ||
               (a == "/SHiP/muon_shield" && b == "/SHiP/neutrino_detector") ||
               (a == "/SHiP/neutrino_detector" && b == "/SHiP/muon_shield");
    };

    // Sort by Z centre
    std::sort(subsystems.begin(), subsystems.end(),
              [](const SubsystemInfo& a, const SubsystemInfo& b) { return a.centreZ < b.centreZ; });

    for (size_t i = 1; i < subsystems.size(); ++i) {
        if (isAllowedOverlap(subsystems[i - 1].name, subsystems[i].name)) {
            continue;
        }

        double previousEnd = subsystems[i - 1].centreZ + subsystems[i - 1].halfZ;
        double currStart = subsystems[i].centreZ - subsystems[i].halfZ;

        INFO("Checking " << subsystems[i - 1].name << " (ends at Z=" << previousEnd << ") vs "
                         << subsystems[i].name << " (starts at Z=" << currStart << ")");
        CHECK(previousEnd <= currStart + 1.0);  // 1 mm tolerance for numerical precision
    }
}

TEST_CASE("ConsistencyTest.PositionsSanity", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);

    // Expected centres derived from subsystem_envelopes.csv (z_start + z_end) / 2, in mm
    // These are approximate — the actual placements may differ slightly from the
    // CSV midpoints due to coordinate system offsets.
    struct Expected {
        std::string name;
        double approxZ;    // mm
        double tolerance;  // mm
    };

    // Centres as placed in SHiPGeometryBuilder::build()
    std::vector<Expected> expected = {
        {"/SHiP/target", 432.5, 500.0},
        {"/SHiP/muon_shield", 16763.3, 500.0},
        {"/SHiP/neutrino_detector", 28950.0, 500.0},
        {"/SHiP/upstream_tagger", 32720.0, 500.0},
        {"/SHiP/decay_volume", 58120.0, 500.0},
        {"/SHiP/trackers", 89570.0, 500.0},
        {"/SHiP/magnet", 89570.0, 500.0},
        {"/SHiP/timing_detector", 95902.0, 500.0},
        {"/SHiP/calorimeter", 98320.0, 500.0},
    };

    for (const auto& exp : expected) {
        auto it = std::find_if(subsystems.begin(), subsystems.end(),
                               [&](const SubsystemInfo& s) { return s.name == exp.name; });

        INFO("Looking for subsystem " << exp.name);
        REQUIRE(it != subsystems.end());
        INFO(exp.name << " at Z=" << it->centreZ << ", expected ~" << exp.approxZ);
        CHECK(std::abs(it->centreZ - exp.approxZ) < exp.tolerance);
    }
}
