// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using SHiPGeometry::assembleGeometry;
using SHiPGeometry::buildSubsystem;
using SHiPGeometry::registry;
using SHiPGeometry::subsystemNames;

namespace {

// The name of the world subsystem, discovered from the registry rather than
// hard-coded: the assembler names no subsystem and neither should the test.
std::string worldName() {
    for (const auto& [name, info] : registry()) {
        if (info.desc.isWorld) {
            return name;
        }
    }
    return {};
}

// A registered subsystem that is not the world, for the single-subsystem and
// selection cases. Returns an empty string if only the world is registered.
std::string someNonWorldName() {
    for (const auto& [name, info] : registry()) {
        if (!info.desc.isWorld) {
            return name;
        }
    }
    return {};
}

// The number of children the world volume owns before any subsystem is placed.
// CavernFactory::build() returns a world that already contains the cavern rock,
// so child counts must be measured against this baseline rather than zero.
// Selecting only the world yields it: assembleGeometry() always builds the
// world, and skips isWorld entries when collecting subsystems to place.
unsigned int worldOwnChildCount() {
    const std::string world = worldName();
    if (world.empty()) {
        return 0;
    }
    GeoPhysVol* bare = assembleGeometry({world});
    return bare ? bare->getNChildVols() : 0;
}

}  // namespace

TEST_CASE("RegistryTest.SubsystemsAreRegistered", "[registry]") {
    // Guards the link configuration as much as the registry: if
    // -Wl,--no-as-needed is dropped, the subsystem libraries fall out of the
    // executable's DT_NEEDED, their static initialisers never run and the
    // registry is silently empty.
    CHECK(registry().size() > 1u);  // NOLINT(readability/check)
}

TEST_CASE("RegistryTest.ExactlyOneWorld", "[registry]") {
    int worlds = 0;
    for (const auto& [name, info] : registry()) {
        if (info.desc.isWorld) {
            INFO("World subsystem: " << name);
            ++worlds;
        }
    }
    CHECK(worlds == 1);  // NOLINT(readability/check)
}

TEST_CASE("RegistryTest.DescriptorIdsAreUnique", "[registry]") {
    // registerSubsystem() rejects duplicate names, but nothing guards the ids.
    // Two subsystems sharing one id would produce colliding GeoIdentifierTags
    // in the assembled tree.
    std::set<int> ids;
    for (const auto& [name, info] : registry()) {
        INFO("Subsystem " << name << " has id " << info.desc.id);
        CHECK(ids.insert(info.desc.id).second);
    }
}

TEST_CASE("RegistryTest.DescriptorNamesMatchRegistryKeys", "[registry]") {
    for (const auto& [key, info] : registry()) {
        INFO("Registry key '" << key << "' vs descriptor name '" << info.desc.name << "'");
        CHECK(key == std::string(info.desc.name));
    }
}

TEST_CASE("RegistryTest.SubsystemNamesAreSortedAndComplete", "[registry]") {
    const std::vector<std::string> names = subsystemNames();
    CHECK(names.size() == registry().size());
    CHECK(std::is_sorted(names.begin(), names.end()));

    const std::string world = worldName();
    REQUIRE(!world.empty());
    INFO("Expected the world (" << world << ") to be listed too");
    CHECK(std::find(names.begin(), names.end(), world) != names.end());
}

TEST_CASE("RegistryTest.BuildSubsystemReturnsAVolume", "[registry]") {
    const std::string name = someNonWorldName();
    REQUIRE(!name.empty());

    INFO("Building subsystem " << name << " on its own");
    GeoVPhysVol* volume = buildSubsystem(name);
    CHECK(volume != nullptr);
}

TEST_CASE("RegistryTest.BuildSubsystemRejectsUnknownNames", "[registry]") {
    // A typo must fail loudly rather than yield a null or partial geometry.
    CHECK_THROWS_AS(buildSubsystem("NoSuchSubsystem"), std::runtime_error);
}

TEST_CASE("RegistryTest.AssembleGeometryRejectsUnknownNames", "[registry]") {
    CHECK_THROWS_AS(assembleGeometry({"NoSuchSubsystem"}), std::runtime_error);
}

TEST_CASE("RegistryTest.AssembleGeometryPlacesOnlyTheSelection", "[registry]") {
    const std::string name = someNonWorldName();
    REQUIRE(!name.empty());

    const unsigned int baseline = worldOwnChildCount();

    GeoPhysVol* world = assembleGeometry({name});
    REQUIRE(world != nullptr);

    // The world is always built; the selection controls what is added to it.
    INFO("Selecting " << name << " should add one child to the world's own " << baseline);
    CHECK(world->getNChildVols() == baseline + 1);
}

TEST_CASE("RegistryTest.EmptySelectionPlacesEveryNonWorldSubsystem", "[registry]") {
    const unsigned int baseline = worldOwnChildCount();

    GeoPhysVol* world = assembleGeometry();
    REQUIRE(world != nullptr);

    const auto expected = baseline + static_cast<unsigned int>(registry().size() - 1);
    INFO("Registry holds " << registry().size() << " subsystems including the world, which owns "
                           << baseline << " child(ren) of its own");
    CHECK(world->getNChildVols() == expected);
}

TEST_CASE("RegistryTest.PlacementOrderIsDeterministic", "[registry]") {
    // assembleGeometry() sorts by (z, id), so the child order must not depend
    // on registration order. Two independent assemblies must agree.
    GeoPhysVol* first = assembleGeometry();
    GeoPhysVol* second = assembleGeometry();
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->getNChildVols() == second->getNChildVols());

    for (unsigned int i = 0; i < first->getNChildVols(); ++i) {
        const std::string a = first->getChildVol(i)->getLogVol()->getName();
        const std::string b = second->getChildVol(i)->getLogVol()->getName();
        INFO("Child " << i << ": '" << a << "' vs '" << b << "'");
        CHECK(a == b);
    }
}

TEST_CASE("RegistryTest.PlacementsFollowDescriptorZOrder", "[registry]") {
    // Measured from the world's own children onwards: the cavern rock is placed
    // by CavernFactory::build() itself and is never sorted by assembleGeometry(),
    // so including it would test the cavern z-origin rather than the ordering.
    const unsigned int baseline = worldOwnChildCount();

    GeoPhysVol* world = assembleGeometry();
    REQUIRE(world != nullptr);
    REQUIRE(world->getNChildVols() >= baseline + 2u);

    double previousZ = -std::numeric_limits<double>::max();
    for (unsigned int i = baseline; i < world->getNChildVols(); ++i) {
        const double z = world->getXToChildVol(i).translation().z();
        INFO("Child " << i << " (" << world->getChildVol(i)->getLogVol()->getName()
                      << ") at z=" << z << ", previous at z=" << previousZ);
        CHECK(z >= previousZ);
        previousZ = z;
    }
}
