// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// The assembler. It contains NO subsystem names: every subsystem registers
// itself (see REGISTER_SUBSYSTEM in each subsystem's .cpp) and this file only
// iterates whatever registered. Each entry point calls linkSubsystems() only so
// that the subsystem libraries stay linked; see src/CMakeLists.txt.
#include "SHiP/geometry/DetectorBuilder.h"

// Not used here: included so the library build runs the layout static_asserts
// against the descriptors the registry places from.
#include "LinkSubsystems.h"
#include "SHiP/geometry/Layout.h"
#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/Placement.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
namespace SHiP::geometry {
GeoPhysVol* assembleGeometry(const std::vector<std::string>& only) {
    detail::linkSubsystems();
    Materials materials;
    auto& reg = registry();
    // Validate any requested names up front, so a typo fails clearly rather
    // than silently producing a partial geometry.
    for (const auto& name : only) {
        if (reg.find(name) == reg.end()) {
            throw std::runtime_error("Unknown subsystem: '" + name + "'");
        }
    }
    // Require exactly one world (isWorld) subsystem, and require it to build a
    // GeoPhysVol (needed to hold child volumes). Two isWorld entries would
    // otherwise be silently resolved to whichever comes first; a mis-typed world
    // factory would make the cast return nullptr. Fail with the offending
    // name(s) instead. The build is deferred until after validation.
    const SubsystemInfo* worldInfo = nullptr;
    for (const auto& [regName, info] : reg) {
        if (!info.desc.isWorld) {
            continue;
        }
        if (worldInfo) {
            throw std::runtime_error("More than one world (isWorld) subsystem registered: '" +
                                     std::string(worldInfo->desc.name) + "' and '" +
                                     std::string(info.desc.name) + "'.");
        }
        worldInfo = &info;
    }
    if (!worldInfo) {
        throw std::runtime_error("No world (isWorld) subsystem is registered");
    }
    GeoPhysVol* world = dynamic_cast<GeoPhysVol*>(worldInfo->build(materials));
    if (!world) {
        throw std::runtime_error("World subsystem '" + std::string(worldInfo->desc.name) +
                                 "' did not build a GeoPhysVol (needed to hold children).");
    }
    // Gather the selected non-world subsystems (all, if none requested), then
    // order them deterministically by (z, id) — independent of registration order.
    std::vector<const SubsystemInfo*> placed;
    for (auto& entry : reg) {
        const SubsystemInfo& info = entry.second;
        if (info.desc.isWorld)
            continue;
        if (only.empty() || std::find(only.begin(), only.end(), entry.first) != only.end()) {
            placed.push_back(&info);
        }
    }
    std::sort(placed.begin(), placed.end(), [](const SubsystemInfo* a, const SubsystemInfo* b) {
        if (a->desc.z_mm != b->desc.z_mm)
            return a->desc.z_mm < b->desc.z_mm;
        return a->desc.id < b->desc.id;
    });
    for (const SubsystemInfo* info : placed) {
        const SubsystemDescriptor& d = info->desc;
        GeoVPhysVol* volume = info->build(materials);
        if (!volume) {
            throw std::runtime_error("Subsystem '" + std::string(d.name) +
                                     "' built a null volume.");
        }
        placeChild(world, volume, d.node, d.id, GeoTrf::Translate3D(d.x_mm, d.y_mm, d.z_mm));
    }
    return world;
}
GeoVPhysVol* buildSubsystem(const std::string& name) {
    detail::linkSubsystems();
    auto& reg = registry();
    auto it = reg.find(name);
    if (it == reg.end()) {
        throw std::runtime_error("Unknown subsystem: '" + name + "'");
    }
    Materials materials;
    return it->second.build(materials);
}
std::vector<std::string> subsystemNames() {
    detail::linkSubsystems();
    std::vector<std::string> names;
    for (auto& entry : registry())
        names.push_back(entry.first);
    return names;
}
// ── Backwards-compatible builder: unchanged interface, delegates to registry ──
DetectorBuilder::DetectorBuilder() = default;
DetectorBuilder::~DetectorBuilder() = default;
GeoPhysVol* DetectorBuilder::build() {
    return assembleGeometry();
}
}  // namespace SHiP::geometry
