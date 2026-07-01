// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoPhysVol.h>  // complete GeoPhysVol/GeoVPhysVol for the macro's upcast

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief A subsystem's self-description: everything the assembler needs.
 *
 * This is the "required input to the geometry builder" that each subsystem
 * yields about itself. It is a plain data type (no GeoModel dependency).
 * Translations are in millimetres from the world origin.
 */
struct SubsystemDescriptor {
    const char* name;      ///< registry key / CLI name, e.g. "Calorimeter"
    const char* node;      ///< GeoNameTag, e.g. "/SHiP/calorimeter"
    int id;                ///< GeoIdentifierTag
    double x_mm;           ///< placement translation X (mm)
    double y_mm;           ///< placement translation Y (mm)
    double z_mm;           ///< placement translation Z, beam direction (mm)
    bool isWorld = false;  ///< true for the mother/world volume (the cavern)
};

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

/// Add a subsystem to the registry. Returns true (usable as a static initialiser).
inline bool registerSubsystem(const SubsystemDescriptor& desc,
                              std::function<GeoVPhysVol*(SHiPMaterials&)> build) {
    registry().emplace(desc.name, SubsystemInfo{desc, std::move(build)});
    return true;
}

// ── Generic consumers — these name no subsystem ─────────────────────────────

/// Assemble the world plus a selection of subsystems (empty selection = all),
/// each placed at its own declared position. Returns the world volume.
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
 * NOTE: because this registration lives in a static library and nothing else
 * references it, the subsystem archive must be linked with --whole-archive
 * (or an equivalent) so the initialiser is not dropped by the linker.
 */
#define REGISTER_SUBSYSTEM(FACTORY)                                                             \
    namespace {                                                                                 \
    const bool FACTORY##_registered = ::SHiPGeometry::registerSubsystem(                        \
        FACTORY::descriptor(), [](::SHiPGeometry::SHiPMaterials& materials) -> ::GeoVPhysVol* { \
            return FACTORY(materials).build();                                                  \
        });                                                                                     \
    }
