// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/DetectorBuilder.h"
#include "SHiP/geometry/Layout.h"
#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/Units.h"

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

using SHiP::geometry::DetectorBuilder;
namespace Layout = SHiP::geometry::Layout;
namespace units = SHiP::geometry::units;
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
    DetectorBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    checkRightHanded(world, world->getLogVol()->getName());
}

TEST_CASE("ConsistencyTest.ExpectedSubsystemCount", "[consistency]") {
    DetectorBuilder builder;
    GeoPhysVol* world = builder.build();
    REQUIRE(world != nullptr);

    auto subsystems = collectSubsystems(world);
    CHECK(subsystems.size() == Layout::kSlots.size());  // NOLINT(readability/check)
}

// Ordering, overlaps and envelope fits are static_asserts in Layout.h; this
// checks that the builder actually places each container at its slot with the
// extents the layout assumed.
TEST_CASE("ConsistencyTest.SubsystemsPlacedAsInLayout", "[consistency]") {
    DetectorBuilder builder;
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
// The SND moved 1 m upstream, deeper into the muon shield.
constexpr auto kSndUpstream = [] {
    auto slots = Layout::kSlots;
    slots[2].z = slots[2].z - 1000.0 * units::mm;
    return slots;
}();
constexpr std::array<Layout::KnownOverlap, 0> kNoKnownOverlaps{};
}  // namespace

static_assert(Layout::firstOutOfOrder(kSwapped) == 3);
static_assert(Layout::firstUnexpectedOverlap(kMagnetOnTiming) != SHiP::geometry::checks::npos);
static_assert(Layout::firstUnexpectedOverlap(Layout::kSlots, kNoKnownOverlaps) == 1,
              "muon shield / SND is the first known overlap");
static_assert(Layout::firstUnexpectedOverlap(kSndUpstream) == 1,
              "a known overlap that grows is no longer accepted");
static_assert(!Layout::fitsEnvelopeZ(Slot{.z = Layout::kDecayVolume.z + 1.0 * units::mm,
                                          .halfZ = Layout::kDecayVolume.halfZ},
                                     SHiP::geometry::Envelopes::kDecayVolume));
static_assert(!SHiP::geometry::checks::overlaps(
                  SHiP::geometry::checks::Span{0.0 * units::mm, 1.0 * units::mm},
                  SHiP::geometry::checks::Span{1.0 * units::mm, 2.0 * units::mm}),
              "touching spans do not overlap");

namespace {
using SHiP::geometry::checks::bounds;
// An L in x-y: the corner x > 1, y > 1 is rock.
constexpr std::array kL{bounds(0, 2, 0, 1, 0, 1), bounds(0, 1, 1, 2, 0, 1)};
}  // namespace

static_assert(SHiP::geometry::checks::coveredBy(bounds(0, 1, 0, 2, 0, 1), kL),
              "a box spanning two halls is covered");
static_assert(!SHiP::geometry::checks::coveredBy(bounds(0, 2, 0, 2, 0, 1), kL),
              "the missing corner of the L is found");
static_assert(!SHiP::geometry::checks::coveredBy(bounds(0, 1, 0, 1, 0, 2), kL),
              "a box poking out along z is not covered");
// The calorimeter used to end 442 mm past the experiment hall.
static_assert(Layout::kCalorimeter.box().z.hi <= Layout::kCavities[1].z.hi,
              "the calorimeter ends inside the experiment hall");

namespace {
constexpr auto kUnit = bounds(0, 1, 0, 1, 0, 1);
}  // namespace

static_assert(SHiP::geometry::checks::overlapsWithin(kUnit, bounds(0.5, 2, 0, 1, 0, 1),
                                                     bounds(0.5, 1, 0, 1, 0, 1)),
              "an overlap inside its region is accepted");
static_assert(!SHiP::geometry::checks::overlapsWithin(kUnit, bounds(0.4, 2, 0, 1, 0, 1),
                                                      bounds(0.5, 1, 0, 1, 0, 1)),
              "an overlap that grows past its region is rejected");
static_assert(!SHiP::geometry::checks::overlapsWithin(kUnit, bounds(1, 2, 0, 1, 0, 1),
                                                      bounds(0.5, 1, 0, 1, 0, 1)),
              "boxes that only touch are not a known overlap");
static_assert(SHiP::geometry::checks::coveredBy(
                  SHiP::geometry::checks::shrunk(bounds(0, 4, 0, 4, -1, 4), 1.0 * units::mm),
                  std::array{bounds(0, 4, 0, 4, 0, 4)}),
              "a box sticking out by 1 is within 1 of the region");
static_assert(!SHiP::geometry::checks::coveredBy(
                  SHiP::geometry::checks::shrunk(bounds(0, 4, 0, 4, -1.5, 4), 1.0 * units::mm),
                  std::array{bounds(0, 4, 0, 4, 0, 4)}),
              "a box sticking out by 1.5 is not");
