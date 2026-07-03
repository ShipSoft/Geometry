// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/DecayVolumeFactory.h"

#include "DecayVolume/SBTConfig.h"
#include "DecayVolume/SBTSensorBuilder.h"
#include "DecayVolume/SBTStructureBuilder.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/Units.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

// Source-tree / install-time fallbacks for sbt.toml, set by CMake.
#ifndef SBT_TOML_DEFAULT_PATH
#define SBT_TOML_DEFAULT_PATH "sbt.toml"
#endif
#ifndef SBT_TOML_INSTALL_PATH
#define SBT_TOML_INSTALL_PATH ""
#endif

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

namespace {

bool fileExists(const std::string& p) {
    std::ifstream f(p);
    return f.good();
}

// Find sbt.toml even when the CWD doesn't contain a copy of it.
std::string resolveTomlPath(const std::string& path) {
    if (fileExists(path))
        return path;
    const std::string srcFallback = SBT_TOML_DEFAULT_PATH;
    if (!srcFallback.empty() && fileExists(srcFallback))
        return srcFallback;
    const std::string installFallback = SBT_TOML_INSTALL_PATH;
    if (!installFallback.empty() && fileExists(installFallback))
        return installFallback;
    return path;  // give up — readSBTConfig will emit the error
}

}  // namespace

DecayVolumeFactory::DecayVolumeFactory(SHiPMaterials& materials, std::string configPath)
    : m_materials(materials), m_configPath(std::move(configPath)) {}

GeoPhysVol* DecayVolumeFactory::build() {
    const SBTConfig cfg = readSBTConfig(resolveTomlPath(m_configPath));

    const GeoMaterial* air = m_materials.requireMaterial(cfg.material_air);
    const GeoMaterial* steel = m_materials.requireMaterial(cfg.material_steel);
    const GeoMaterial* alMat = m_materials.requireMaterial(cfg.material_wall);
    const GeoMaterial* labMat = m_materials.requireMaterial(cfg.material_cell);
    const GeoMaterial* helium = m_materials.requireMaterial(cfg.material_helium);

    // ── Air container ────────────────────────────────────────────────────
    // The container is the experiment's fixed envelope allocation for the
    // decay region (s_halfX/Y/Z). The SBT geometry comes from sbt.toml, so a
    // future edit there could in principle outgrow it. Guard against that:
    // bound the outermost structure reach from the config and fail loudly if
    // it would pierce the envelope, rather than silently overlapping.
    //
    // Reference description: rectangular frustum, 2.0 x 3.0 m entrance ->
    // 4.0 x 6.0 m exit, 50 m long. With the default config the reaches are
    // ~2130 / ~3259 / ~25130 mm, inside the 2200 / 3300 / 25200 envelope.
    {
        const double growthY = (cfg.y_half_exit_mm - cfg.y_half_entrance_mm) / cfg.total_length_mm;
        // X: vertical columns sit at x_half_exit with a half-flange overhang.
        const double reachX = cfg.x_half_exit_mm + 0.5 * cfg.hbeam_flange_width_mm;
        // Y: top/bottom cross-beams are shifted a full beam-height above
        // y_half_exit (plus a small frustum-growth term); +10 pads the
        // assembly standoff and is a safe upper bound on the real ~3259 mm.
        const double reachY = cfg.y_half_exit_mm + cfg.hbeam_height_mm +
                              0.5 * cfg.hbeam_flange_width_mm * growthY + 10.0;
        // Z: the 50 m structure spans [z_entrance, z_entrance + total_length];
        // cross-beam flanges extend half a flange-width beyond the end rows.
        const double reachZ = std::max(std::abs(cfg.z_entrance_mm),
                                       std::abs(cfg.z_entrance_mm + cfg.total_length_mm)) +
                              0.5 * cfg.hbeam_flange_width_mm;

        if (reachX > s_halfX || reachY > s_halfY || reachZ > s_halfZ) {
            throw std::runtime_error(
                "DecayVolumeFactory: configured SBT (half-reach " + std::to_string(reachX) + " x " +
                std::to_string(reachY) + " x " + std::to_string(reachZ) +
                " mm) does not fit the decay-volume envelope (" + std::to_string(s_halfX) + " x " +
                std::to_string(s_halfY) + " x " + std::to_string(s_halfZ) +
                " mm); adjust sbt.toml or the container size.");
        }
    }

    auto* containerBox = new GeoBox(s_halfX * mm, s_halfY * mm, s_halfZ * mm);
    auto* containerLog = new GeoLogVol("/SHiP/decay_volume", containerBox, air);
    auto* container = new GeoPhysVol(containerLog);

    // ── Central helium decay region ──────────────────────────────────────
    // A frustum sized inside the innermost sensor faces (x_half - thickness,
    // y_half - thickness) minus the helium clearance, centred on the SBT.
    const double inset = cfg.container_thickness_mm + cfg.helium_clearance_mm;
    const double dz = 0.5 * cfg.total_length_mm * mm;
    const double dx1 = (cfg.x_half_entrance_mm - inset) * mm;
    const double dy1 = (cfg.y_half_entrance_mm - inset) * mm;
    const double dx2 = (cfg.x_half_exit_mm - inset) * mm;
    const double dy2 = (cfg.y_half_exit_mm - inset) * mm;

    auto* heShape = new GeoTrap(dz, 0.0, 0.0, dy1, dx1, dx1, 0.0, dy2, dx2, dx2, 0.0);
    auto* heLog = new GeoLogVol("/SHiP/decay_volume/helium", heShape, helium);
    auto* hePhys = new GeoPhysVol(heLog);
    container->add(new GeoNameTag("/SHiP/decay_volume/helium"));
    container->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, 0.0)));
    container->add(hePhys);

    // ── SBT steel structure + LAB sensors, wrapping the helium ───────────
    SBTStructureBuilder::build(container, steel, cfg);
    SBTSensorBuilder::build(container, alMat, labMat, cfg);

    return container;
}

}  // namespace SHiPGeometry
