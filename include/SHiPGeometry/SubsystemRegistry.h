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

/**
 * @brief A registered subsystem: its descriptor plus how to build it.
 *
 * One entry per subsystem in the global registry(). The callback builds the
 * subsystem in its own local frame; the descriptor says where that frame goes
 * in the world.
 */
struct SubsystemInfo {
    SubsystemDescriptor desc;                           ///< name, tree node, id and placement
    std::function<GeoVPhysVol*(SHiPMaterials&)> build;  ///< builds it in its local frame
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

/**
 * @brief Add a subsystem to the registry.
 *
 * A duplicate name is a programming error (two subsystems declaring the same
 * descriptor name) and aborts with a diagnostic, rather than being silently
 * dropped by emplace(). This runs during static initialisation, before main()
 * and before any handler could catch it, so it reports to stderr and calls
 * std::abort() instead of throwing. That also keeps this header free of any
 * logging dependency, which would otherwise become public to every consumer.
 *
 * @param desc  The subsystem's self-description; desc.name must be unique.
 * @param build Callback building the subsystem in its own local frame.
 * @return Always true, so the call is usable as a static initialiser.
 */
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

/**
 * @brief Assemble the world plus a selection of registered subsystems.
 *
 * The world (the registered subsystem whose descriptor sets isWorld) is always
 * built. Every selected subsystem is placed into it at the translation its own
 * descriptor declares, in a deterministic order sorted by (z, id) so the result
 * does not depend on registration order.
 *
 * @param only Subsystem names to place; an empty selection places all of them.
 * @return The world physical volume, owning the placed subsystems.
 * @throws std::runtime_error if a name is not registered, if no world is
 *         registered, if more than one is, or if a factory builds nothing.
 */
GeoPhysVol* assembleGeometry(const std::vector<std::string>& only = {});

/**
 * @brief Build a single subsystem on its own, in its local frame.
 *
 * No world is created and no placement is applied, so the result sits at the
 * origin rather than at its position in the detector.
 *
 * @param name The subsystem name, as returned by subsystemNames().
 * @return The subsystem's volume in its local frame.
 * @throws std::runtime_error if the name is not registered.
 */
GeoVPhysVol* buildSubsystem(const std::string& name);

/**
 * @brief The names of every registered subsystem, sorted.
 *
 * Includes the world, so the result is the full set of names accepted by
 * buildSubsystem() and assembleGeometry().
 *
 * @return Sorted subsystem names.
 */
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
    [[maybe_unused]] const bool FACTORY##_registered = ::SHiPGeometry::registerSubsystem(       \
        FACTORY::descriptor(), [](::SHiPGeometry::SHiPMaterials& materials) -> ::GeoVPhysVol* { \
            return FACTORY(materials).build();                                                  \
        });                                                                                     \
    }
