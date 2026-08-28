// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/SubsystemDescriptor.h"

#include <GeoModelKernel/GeoPhysVol.h>  // complete GeoPhysVol/GeoVPhysVol for the macro's upcast

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace SHiPGeometry {

class SHiPMaterials;

/// A registered subsystem: its descriptor plus how to build it (local frame).
struct SubsystemInfo {
    SubsystemDescriptor desc;
    std::function<GeoVPhysVol*(SHiPMaterials&)> build;
};

/**
 * @brief The global subsystem registry.
 *
 * A Meyers singleton (function-local static in an inline function) so it is
 * a single shared instance across all translation units and is guaranteed to
 * exist before any static registration runs. No file names any subsystem;
 * subsystems add themselves via REGISTER_SUBSYSTEM.
 */
inline std::map<std::string, SubsystemInfo>& registry() {
    static std::map<std::string, SubsystemInfo> instance;
    return instance;
}

/// Add a subsystem to the registry. Returns true (usable as a static
/// initialiser). A duplicate name is a programming error (two subsystems
/// declaring the same descriptor name): it is reported and aborts, rather
/// than being silently dropped by emplace(). Runs at static-init, so this
/// diagnoses to stderr and aborts instead of throwing.
inline bool registerSubsystem(const SubsystemDescriptor& desc,
                              std::function<GeoVPhysVol*(SHiPMaterials&)> build) {
    const auto result = registry().emplace(desc.name, SubsystemInfo{desc, std::move(build)});
    if (!result.second) {
        std::fprintf(stderr,
                     "SHiPGeometry: duplicate subsystem name '%s' registered; "
                     "each subsystem's descriptor() must return a unique name.\n",
                     desc.name);
        std::abort();
    }
    return true;
}

// ── Generic consumers — these name no subsystem ─────────────────────────────

/// Assemble the world plus a selection of subsystems (empty selection = all),
/// each placed at its own declared position. Unknown names throw
/// std::runtime_error. Returns the world volume.
GeoPhysVol* assembleGeometry(const std::vector<std::string>& only = {});

/// Build a single subsystem on its own, in its local frame (no world, no
/// placement). Throws std::runtime_error if the name is not registered.
GeoVPhysVol* buildSubsystem(const std::string& name);

/// The names of all registered subsystems (including the world), sorted.
std::vector<std::string> subsystemNames();

}  // namespace SHiPGeometry

/**
 * @brief Register a subsystem factory with the global registry.
 *
 * Placed once in each subsystem's own .cpp (inside namespace SHiPGeometry).
 * The factory must expose `static SubsystemDescriptor descriptor()` and be
 * constructible from `SHiPMaterials&` with a `build()` returning a volume.
 *
 * NOTE: nothing references this registration, so the subsystem library would
 * otherwise be dropped from a consumer's DT_NEEDED by the toolchain's default
 * --as-needed and the initialiser would never run. src/CMakeLists.txt applies
 * -Wl,--no-as-needed as an INTERFACE link option on SHiPGeometry so the flag
 * lands on each executable's link line. Do not remove it.
 */
#define REGISTER_SUBSYSTEM(FACTORY)                                                             \
    namespace {                                                                                 \
    const bool FACTORY##_registered = ::SHiPGeometry::registerSubsystem(                        \
        FACTORY::descriptor(), [](::SHiPGeometry::SHiPMaterials& materials) -> ::GeoVPhysVol* { \
            return FACTORY(materials).build();                                                  \
        });                                                                                     \
    }
