// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIntrusivePtr.h>
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

using SHiP::geometry::assembleGeometry;
using SHiP::geometry::buildSubsystem;
using SHiP::geometry::registry;
using SHiP::geometry::subsystemNames;

#ifndef SHIP_EXPECTED_SUBSYSTEMS
#error "SHIP_EXPECTED_SUBSYSTEMS must be defined by tests/CMakeLists.txt"
#endif

namespace {

// The build's SHIP_SUBSYSTEMS list (comma-joined by tests/CMakeLists.txt),
// sorted to compare against subsystemNames().
std::vector<std::string> expectedSubsystems() {
    const std::string list = SHIP_EXPECTED_SUBSYSTEMS;
    std::vector<std::string> names;
    std::string::size_type start = 0;
    while (start <= list.size()) {
        const std::string::size_type end = std::min(list.find(',', start), list.size());
        names.push_back(list.substr(start, end - start));
        start = end + 1;
    }
    std::sort(names.begin(), names.end());
    return names;
}

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
// Computed once, since each call would build the cavern again.
unsigned int worldOwnChildCount() {
    static const unsigned int count = [] {
        const std::string world = worldName();
        if (world.empty()) {
            return 0u;
        }
        const GeoIntrusivePtr<GeoPhysVol> bare{assembleGeometry({world})};
        return bare.get() ? bare->getNChildVols() : 0u;
    }();
    return count;
}

// The complete detector, assembled once and shared by the tests that only
// read it: building it takes most of this test's run time.
const GeoPhysVol* fullAssembly() {
    static const GeoIntrusivePtr<GeoPhysVol> world{assembleGeometry()};
    return world.get();
}

// Adds a subsystem to the registry for the lifetime of the guard, so that a
// test can control the descriptors the assembler sees. Removed again on
// destruction, so later tests see the real registry only.
class StubSubsystem {
   public:
    StubSubsystem(const char* name, int id, double z_mm) : m_name(name) {
        SHiP::geometry::registerSubsystem(
            {name, name, id, 0.0, 0.0, z_mm, false},
            [name](SHiP::geometry::Materials& materials) -> GeoVPhysVol* {
                auto* box = new GeoBox(10.0, 10.0, 10.0);
                auto* log = new GeoLogVol(name, box, materials.getMaterial("Air"));
                return new GeoPhysVol(log);
            });
    }
    ~StubSubsystem() { registry().erase(m_name); }
    StubSubsystem(const StubSubsystem&) = delete;
    StubSubsystem& operator=(const StubSubsystem&) = delete;
    StubSubsystem(StubSubsystem&&) = delete;
    StubSubsystem& operator=(StubSubsystem&&) = delete;

   private:
    std::string m_name;
};

}  // namespace

TEST_CASE("RegistryTest.SubsystemsAreRegistered", "[registry]") {
    // Guards the link configuration as much as the registry: if the linker
    // drops the subsystem libraries, their static initialisers never run and
    // the registry is silently empty.
    CHECK(registry().size() > 1u);  // NOLINT(readability/check)
}

TEST_CASE("RegistryTest.EveryBuiltSubsystemIsRegistered", "[registry]") {
    // Every library in SHIP_SUBSYSTEMS must have registered itself under its
    // directory name. A forgotten REGISTER_SUBSYSTEM, or a descriptor() whose
    // name differs from the directory, shows up here as a mismatch.
    CHECK(subsystemNames() == expectedSubsystems());
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
    const GeoIntrusivePtr<GeoVPhysVol> volume{buildSubsystem(name)};
    CHECK(volume.get() != nullptr);
}

TEST_CASE("RegistryTest.BuildSubsystemRejectsUnknownNames", "[registry]") {
    // A typo must fail loudly rather than yield a null or partial geometry.
    CHECK_THROWS_AS(buildSubsystem("NoSuchSubsystem"), SHiP::geometry::UnknownSubsystem);
}

TEST_CASE("RegistryTest.AssembleGeometryRejectsUnknownNames", "[registry]") {
    CHECK_THROWS_AS(assembleGeometry({"NoSuchSubsystem"}), SHiP::geometry::UnknownSubsystem);
}

TEST_CASE("RegistryTest.UnknownSubsystemCarriesTheName", "[registry]") {
    try {
        (void)buildSubsystem("NoSuchSubsystem");
        FAIL("buildSubsystem() accepted an unknown name");
    } catch (const SHiP::geometry::UnknownSubsystem& e) {
        CHECK(e.name() == "NoSuchSubsystem");
    }
}

TEST_CASE("RegistryTest.AssembleGeometryPlacesOnlyTheSelection", "[registry]") {
    const std::string name = someNonWorldName();
    REQUIRE(!name.empty());

    const unsigned int baseline = worldOwnChildCount();

    const GeoIntrusivePtr<GeoPhysVol> world{assembleGeometry({name})};
    REQUIRE(world.get() != nullptr);

    // The world is always built; the selection controls what is added to it.
    INFO("Selecting " << name << " should add one child to the world's own " << baseline);
    CHECK(world->getNChildVols() == baseline + 1);
}

TEST_CASE("RegistryTest.EmptySelectionPlacesEveryNonWorldSubsystem", "[registry]") {
    const unsigned int baseline = worldOwnChildCount();

    const GeoPhysVol* world = fullAssembly();
    REQUIRE(world != nullptr);

    const auto expected = baseline + static_cast<unsigned int>(registry().size() - 1);
    INFO("Registry holds " << registry().size() << " subsystems including the world, which owns "
                           << baseline << " child(ren) of its own");
    CHECK(world->getNChildVols() == expected);
}

TEST_CASE("RegistryTest.EqualZIsOrderedById", "[registry]") {
    // The registry iterates by name, so without the (z, id) sort AAStub would
    // be placed first. With it, the lower id wins the tie on z.
    const StubSubsystem first{"AAStub", 9001, 0.0};
    const StubSubsystem second{"ZZStub", 9000, 0.0};
    const unsigned int baseline = worldOwnChildCount();

    const GeoIntrusivePtr<GeoPhysVol> world{assembleGeometry({"AAStub", "ZZStub"})};
    REQUIRE(world.get() != nullptr);
    REQUIRE(world->getNChildVols() == baseline + 2u);

    CHECK(world->getChildVol(baseline)->getLogVol()->getName() == "ZZStub");
    CHECK(world->getChildVol(baseline + 1)->getLogVol()->getName() == "AAStub");
}

TEST_CASE("RegistryTest.PlacementsFollowDescriptorZOrder", "[registry]") {
    // Measured from the world's own children onwards: the cavern rock is placed
    // by CavernFactory::build() itself and is never sorted by assembleGeometry(),
    // so including it would test the cavern z-origin rather than the ordering.
    const unsigned int baseline = worldOwnChildCount();

    const GeoPhysVol* world = fullAssembly();
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
