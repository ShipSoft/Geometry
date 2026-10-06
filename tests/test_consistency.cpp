// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/Layout.h"
#include "SHiPGeometry/SHiPGeometry.h"
#include "SHiPGeometry/StaticChecks.h"
#include "SHiPGeometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using SHiPGeometry::SHiPGeometryBuilder;
namespace Layout = SHiPGeometry::Layout;
namespace units = SHiPGeometry::units;
using units::gm;

namespace {

// Names of the world's subsystem children (the Cavern rock is skipped).
std::vector<std::string> collectSubsystems(const GeoPhysVol* world) {
    std::vector<std::string> names;
    for (unsigned int i = 0; i < world->getNChildVols(); ++i) {
        std::string name = world->getChildVol(i)->getLogVol()->getName();
        if (name != "/SHiP/cavern") {
            names.push_back(std::move(name));
        }
    }
    return names;
}

struct FoundChild {
    const GeoVPhysVol* vol;
    unsigned int index;
};

std::optional<FoundChild> findChild(const GeoPhysVol* parent, std::string_view name) {
    for (unsigned int i = 0; i < parent->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*parent->getChildVol(i);
        if (child->getLogVol()->getName() == name) {
            return FoundChild{child, i};
        }
    }
    return std::nullopt;
}

// Recursively assert every placement transform is a pure rotation (no
// reflection): the linear part of getXToChildVol must have positive
// determinant. Guards against left-handed frames like the one fixed in PR #70
// (SBTStructureBuilder), which Geant4 rejects as improper rotations.
void checkRightHanded(const GeoVPhysVol* vol, const std::string& path = "") {
    for (unsigned int i = 0; i < vol->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*vol->getChildVol(i);
        const double det = vol->getXToChildVol(i).linear().determinant();
        const std::string childPath =
            path + "/[" + std::to_string(i) + "]" + child->getLogVol()->getName();
        INFO("Placement " << childPath << " has determinant " << det);
        CHECK(det > 0.0);
        checkRightHanded(child, childPath);
    }
}

}  // namespace

TEST_CASE("ConsistencyTest.AllRotationsRightHanded", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    checkRightHanded(world, world->getLogVol()->getName());
}

TEST_CASE("ConsistencyTest.ExpectedSubsystemCount", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);
    CHECK(subsystems.size() == Layout::kSlots.size());  // NOLINT(readability/check)
}

// Ordering, overlaps and envelope fits are static_asserts in Layout.h; this
// checks that the builder actually places each container at its slot with the
// extents the layout assumed.
TEST_CASE("ConsistencyTest.SubsystemsPlacedAsInLayout", "[consistency]") {
    SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    for (const auto& slot : Layout::kSlots) {
        INFO("Subsystem " << slot.path);
        const auto child = findChild(world, slot.path);
        REQUIRE(child.has_value());
        const GeoTrf::Vector3D t = world->getXToChildVol(child->index).translation();
        CHECK(t.x() == gm(slot.x));
        CHECK(t.y() == gm(slot.y));
        CHECK(t.z() == gm(slot.z));

        const auto* box = dynamic_cast<const GeoBox*>(child->vol->getLogVol()->getShape());
        REQUIRE(box != nullptr);
        CHECK(box->getXHalfLength() == gm(slot.halfX));
        CHECK(box->getYHalfLength() == gm(slot.halfY));
        CHECK(box->getZHalfLength() == gm(slot.halfZ));
    }
}

// Negative controls for the Layout.h predicates: each must reject a layout
// that is broken in the way it is meant to catch.
namespace {
using Layout::Slot;
constexpr auto kSwapped = [] {
    auto slots = Layout::kSlots;
    std::swap(slots[3], slots[4]);
    return slots;
}();
constexpr auto kMagnetOnTiming = [] {
    auto slots = Layout::kSlots;
    slots[6].z = Layout::kTimingDetector.z;
    return slots;
}();
constexpr std::array<std::array<std::string_view, 2>, 0> kNoOverlapsAllowed{};
}  // namespace

static_assert(Layout::firstOutOfOrder(kSwapped) == 3);
static_assert(Layout::firstUnexpectedOverlap(kMagnetOnTiming) != SHiPGeometry::checks::npos);
static_assert(Layout::firstUnexpectedOverlap(Layout::kSlots, kNoOverlapsAllowed) == 1,
              "muon shield / SND is the first intended overlap");
static_assert(!Layout::fitsEnvelopeZ(Slot{.z = Layout::kDecayVolume.z + 1.0 * units::mm,
                                          .halfZ = Layout::kDecayVolume.halfZ},
                                     SHiPGeometry::Envelopes::kDecayVolume));
static_assert(
    !SHiPGeometry::checks::overlaps(SHiPGeometry::checks::Span{0.0 * units::mm, 1.0 * units::mm},
                                    SHiPGeometry::checks::Span{1.0 * units::mm, 2.0 * units::mm}),
    "touching spans do not overlap");
