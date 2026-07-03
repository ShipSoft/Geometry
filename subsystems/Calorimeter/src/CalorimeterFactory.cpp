// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterFactory.h"

#include "Calorimeter/CaloBarLayer.h"
#include "Calorimeter/CaloFibreHPLayer.h"
#include "Calorimeter/CalorimeterConfig.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

#include <cmath>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>

// Absolute fallback path baked in by CMake so out-of-source builds always
// find calo.toml even when the CWD doesn't contain a copy of it.
#ifndef CALO_TOML_DEFAULT_PATH
#define CALO_TOML_DEFAULT_PATH "calo.toml"
#endif
// Install-time data directory path, set by CMake during install configuration.
#ifndef CALO_TOML_INSTALL_PATH
#define CALO_TOML_INSTALL_PATH ""
#endif

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

// Bar pitch (physical constant, independent of plate size)
static constexpr double kWidePVTBarPitch_mm = 60.0;
static constexpr double kThinPSBarPitch_mm = 10.0;

// ── file-scope helper ────────────────────────────────────────────────────────

static std::string resolveTomlPath(const std::string& path) {
    if (!path.empty() && path[0] == '/')
        return path;  // already absolute
    if (std::filesystem::exists(path))
        return path;  // found relative to CWD
    const std::string srcFallback = CALO_TOML_DEFAULT_PATH;
    if (std::filesystem::exists(srcFallback))
        return srcFallback;  // source-tree fallback
    const std::string installFallback = CALO_TOML_INSTALL_PATH;
    if (!installFallback.empty() && std::filesystem::exists(installFallback))
        return installFallback;  // installed data dir
    return path;                 // give up — readCaloConfig will emit the error
}

// ── constructor / accessors ──────────────────────────────────────────────────

CalorimeterFactory::CalorimeterFactory(SHiPMaterials& materials, std::string configPath)
    : m_materials(materials), m_configPath(std::move(configPath)) {}

// ── totalStackZ ──────────────────────────────────────────────────────────────

double CalorimeterFactory::totalStackZ(const CalorimeterConfig& cfg) {
    auto sectionZ = [&](const std::vector<int>& codes, double absorberThickness) {
        double z = 0.0;
        for (int code : codes) {
            switch (static_cast<LayerCode>(code)) {
                case LayerCode::Absorber:
                    z += absorberThickness;
                    break;
                case LayerCode::WidePVT_H:
                case LayerCode::WidePVT_V:
                case LayerCode::ThinPS_H:
                case LayerCode::ThinPS_V:
                    z += cfg.scint_thickness_mm;
                    break;
                case LayerCode::FibreHPL_Y:
                case LayerCode::FibreHPL_X:
                    z += cfg.hpl_thickness_mm;
                    break;
                case LayerCode::AirGap:
                    z += cfg.airgap_mm;
                    break;
                default:
                    throw std::runtime_error(
                        std::format("CalorimeterFactory: unknown layer code: {}", code));
            }
        }
        return z;
    };
    return sectionZ(cfg.layers, cfg.lead_thickness_mm) + cfg.gap_ecal_hcal_mm +
           sectionZ(cfg.layers2, cfg.iron_thickness_mm);
}

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* CalorimeterFactory::build() {
    const CalorimeterConfig cfg = readCaloConfig(resolveTomlPath(m_configPath));

    GeoMaterial* air = m_materials.requireMaterial("Air");

    // Fixed-size container — must match the SHiP subsystem envelope so that
    // the geometry consistency tests and the overlap check pass.
    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/calorimeter", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Module pitch (0 → use plate size, modules touch)
    const double pitchX = cfg.module_pitch_x_mm > 0.0 ? cfg.module_pitch_x_mm : cfg.plate_xy_mm;
    const double pitchY = cfg.module_pitch_y_mm > 0.0 ? cfg.module_pitch_y_mm : cfg.plate_xy_mm;

    // ── Validate that the config fits inside the fixed container ─────────────
    const double stackZ = totalStackZ(cfg);
    const double maxHalfX = 0.5 * cfg.plate_xy_mm + 0.5 * (cfg.module_nx - 1) * pitchX;
    const double maxHalfY = 0.5 * cfg.plate_xy_mm + 0.5 * (cfg.module_ny - 1) * pitchY;
    if (stackZ > 2.0 * s_containerHalfZ)
        throw std::runtime_error(std::format(
            "CalorimeterFactory: calo.toml total stack Z ({} mm) exceeds container Z ({} mm). "
            "Reduce layer thicknesses or number of layers.",
            stackZ, 2.0 * s_containerHalfZ));
    if (maxHalfX > s_containerHalfX)
        throw std::runtime_error(std::format(
            "CalorimeterFactory: calo.toml module array half-X ({} mm) exceeds container half-X "
            "({} mm). Reduce plate_xy_mm, module_nx, or module_pitch_x_mm.",
            maxHalfX, s_containerHalfX));
    if (maxHalfY > s_containerHalfY)
        throw std::runtime_error(std::format(
            "CalorimeterFactory: calo.toml module array half-Y ({} mm) exceeds container half-Y "
            "({} mm). Reduce plate_xy_mm, module_ny, or module_pitch_y_mm.",
            maxHalfY, s_containerHalfY));

    const double x0 = -0.5 * (cfg.module_nx - 1) * pitchX;
    const double y0 = -0.5 * (cfg.module_ny - 1) * pitchY;

    for (int iy = 0; iy < cfg.module_ny; ++iy)
        for (int ix = 0; ix < cfg.module_nx; ++ix)
            buildStack(containerPhys, cfg, ix, iy, x0 + ix * pitchX, y0 + iy * pitchY);

    return containerPhys;
}

// ── buildStack ───────────────────────────────────────────────────────────────

void CalorimeterFactory::buildStack(GeoPhysVol* container, const CalorimeterConfig& cfg,
                                    int moduleX, int moduleY, double offsetX,
                                    double offsetY) const {
    // Compute bar counts from configurable plate size
    if (std::fmod(cfg.plate_xy_mm, kWidePVTBarPitch_mm) != 0.0)
        throw std::runtime_error(std::format(
            "CalorimeterFactory: plate_xy_mm ({}) is not divisible by wide PVT bar pitch ({} mm)",
            cfg.plate_xy_mm, kWidePVTBarPitch_mm));
    if (std::fmod(cfg.plate_xy_mm, kThinPSBarPitch_mm) != 0.0)
        throw std::runtime_error(std::format(
            "CalorimeterFactory: plate_xy_mm ({}) is not divisible by thin PS bar pitch ({} mm)",
            cfg.plate_xy_mm, kThinPSBarPitch_mm));
    const int widePVTBarCount = static_cast<int>(cfg.plate_xy_mm / kWidePVTBarPitch_mm);
    const int thinPSBarCount = static_cast<int>(cfg.plate_xy_mm / kThinPSBarPitch_mm);

    GeoMaterial* leadMat = m_materials.requireMaterial("Lead");
    GeoMaterial* ironMat = m_materials.requireMaterial("Iron");
    GeoMaterial* pvtMat = m_materials.requireMaterial("PVT");
    GeoMaterial* psMat = m_materials.requireMaterial("Polystyrene");
    GeoMaterial* alMat = m_materials.requireMaterial("Aluminium");
    GeoMaterial* airMat = m_materials.requireMaterial("Air");

    const double plateXY = cfg.plate_xy_mm * mm;
    const double leadZ = cfg.lead_thickness_mm * mm;
    const double scintZ = cfg.scint_thickness_mm * mm;
    const double hplZ = cfg.hpl_thickness_mm * mm;
    const double ironZ = cfg.iron_thickness_mm * mm;
    const double airGapZ = cfg.airgap_mm * mm;

    const double wideW = kWidePVTBarPitch_mm * mm;
    const double thinW = kThinPSBarPitch_mm * mm;

    const std::string moduleTag = std::format("_MX{}Y{}", moduleX, moduleY);

    // Reusable LogVols — GeoModel shares them across GeoPhysVol instances
    auto* leadLog = new GeoLogVol("/SHiP/calorimeter/lead_plate" + moduleTag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * leadZ), leadMat);
    auto* ironLog = new GeoLogVol("/SHiP/calorimeter/iron_plate" + moduleTag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * ironZ), ironMat);
    auto* wideHLog = new GeoLogVol("/SHiP/calorimeter/wide_pvt_h" + moduleTag,
                                   new GeoBox(0.5 * plateXY, 0.5 * wideW, 0.5 * scintZ), pvtMat);
    auto* wideVLog = new GeoLogVol("/SHiP/calorimeter/wide_pvt_v" + moduleTag,
                                   new GeoBox(0.5 * wideW, 0.5 * plateXY, 0.5 * scintZ), pvtMat);
    auto* thinHLog = new GeoLogVol("/SHiP/calorimeter/thin_ps_h" + moduleTag,
                                   new GeoBox(0.5 * plateXY, 0.5 * thinW, 0.5 * scintZ), psMat);
    auto* thinVLog = new GeoLogVol("/SHiP/calorimeter/thin_ps_v" + moduleTag,
                                   new GeoBox(0.5 * thinW, 0.5 * plateXY, 0.5 * scintZ), psMat);

    // z cursor: start at -halfContainerZ if centering, else 0
    // We centre within the fixed container (halfZ = s_containerHalfZ).
    const double totalZ = totalStackZ(cfg);
    double zCursor = cfg.center_stack ? -0.5 * totalZ * mm : -s_containerHalfZ;

    int layerId = 0;  // monotonic identifier used for GeoIdentifierTag

    // Lambda: wrap a layer in a named air-box envelope inside the container
    auto makeEnv = [&](const std::string& name, double halfZ, double zCenter) -> GeoPhysVol* {
        auto* envShape = new GeoBox(0.5 * plateXY, 0.5 * plateXY, halfZ);
        auto* envLog = new GeoLogVol(name + "_env", envShape, airMat);
        auto* envPhys = new GeoPhysVol(envLog);
        container->add(new GeoNameTag(name.c_str()));
        container->add(new GeoIdentifierTag(layerId++));
        container->add(new GeoTransform(GeoTrf::Translate3D(offsetX, offsetY, zCenter)));
        container->add(envPhys);
        return envPhys;
    };

    int iWideH = 0, iWideV = 0, iThinH = 0, iThinV = 0, iHPL = 0;

    // Describes one calorimeter section (ECAL or HCAL)
    struct SectionDescriptor {
        std::string_view prefix;  // "ecal" or "hcal"
        const std::vector<int>& layerCodes;
        GeoLogVol* absorberLog;
        double absorberHalfZ;
        bool absorberNeedsEnvelope;    // ECAL lead: true, HCAL iron: false
        bool incrementGlobalOnAirGap;  // ECAL: false, HCAL: true
    };

    auto processSection = [&](const SectionDescriptor& sec) {
        int globalLayerIdx = 0, scintLayerIdx = 0;
        int absorberIdx = 0;

        for (int code : sec.layerCodes) {
            const auto basePath =
                std::format("/SHiP/calorimeter/{}/gl{}", sec.prefix, globalLayerIdx);

            switch (static_cast<LayerCode>(code)) {
                case LayerCode::Absorber: {
                    if (sec.absorberNeedsEnvelope) {
                        const auto volumeName = std::format("{}_lead{}", basePath, moduleTag);
                        auto* env =
                            makeEnv(volumeName, sec.absorberHalfZ, zCursor + sec.absorberHalfZ);
                        env->add(new GeoNameTag(volumeName.c_str()));
                        env->add(new GeoIdentifierTag(0));
                        env->add(new GeoTransform(GeoTrf::Translate3D(0, 0, 0)));
                        env->add(new GeoPhysVol(sec.absorberLog));
                    } else {
                        const auto volumeName =
                            std::format("{}_iron_{}{}", basePath, absorberIdx, moduleTag);
                        container->add(new GeoNameTag(volumeName.c_str()));
                        container->add(new GeoIdentifierTag(layerId++));
                        container->add(new GeoTransform(
                            GeoTrf::Translate3D(offsetX, offsetY, zCursor + sec.absorberHalfZ)));
                        container->add(new GeoPhysVol(sec.absorberLog));
                    }
                    zCursor += 2.0 * sec.absorberHalfZ;
                    ++absorberIdx;
                    ++globalLayerIdx;
                    break;
                }
                case LayerCode::WidePVT_H: {
                    const auto volumeName =
                        std::format("{}_sl{}_wide_pvt_h{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * scintZ, zCursor + 0.5 * scintZ);
                    CaloBar::placeLayer(env, wideHLog, kWidePVTBarPitch_mm, widePVTBarCount, 0.0,
                                        volumeName.c_str(), iWideH, BarAxis::AlongY, moduleTag);
                    zCursor += scintZ;
                    ++iWideH;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::WidePVT_V: {
                    const auto volumeName =
                        std::format("{}_sl{}_wide_pvt_v{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * scintZ, zCursor + 0.5 * scintZ);
                    CaloBar::placeLayer(env, wideVLog, kWidePVTBarPitch_mm, widePVTBarCount, 0.0,
                                        volumeName.c_str(), iWideV, BarAxis::AlongX, moduleTag);
                    zCursor += scintZ;
                    ++iWideV;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::ThinPS_H: {
                    const auto volumeName =
                        std::format("{}_sl{}_thin_ps_h{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * scintZ, zCursor + 0.5 * scintZ);
                    CaloBar::placeLayer(env, thinHLog, kThinPSBarPitch_mm, thinPSBarCount, 0.0,
                                        volumeName.c_str(), iThinH, BarAxis::AlongY, moduleTag);
                    zCursor += scintZ;
                    ++iThinH;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::ThinPS_V: {
                    const auto volumeName =
                        std::format("{}_sl{}_thin_ps_v{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * scintZ, zCursor + 0.5 * scintZ);
                    CaloBar::placeLayer(env, thinVLog, kThinPSBarPitch_mm, thinPSBarCount, 0.0,
                                        volumeName.c_str(), iThinV, BarAxis::AlongX, moduleTag);
                    zCursor += scintZ;
                    ++iThinV;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::FibreHPL_Y: {
                    const auto volumeName =
                        std::format("{}_sl{}_hpl_y{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * hplZ, zCursor + 0.5 * hplZ);
                    CaloFibreHP::buildLayer(env, alMat, psMat, volumeName, 0.0, iHPL,
                                            cfg.plate_xy_mm, cfg.hpl_thickness_mm,
                                            cfg.fiber_diameter_mm, cfg.fiber_core_diameter_mm, true,
                                            moduleTag);
                    zCursor += hplZ;
                    ++iHPL;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::FibreHPL_X: {
                    const auto volumeName =
                        std::format("{}_sl{}_hpl_x{}", basePath, scintLayerIdx, moduleTag);
                    auto* env = makeEnv(volumeName, 0.5 * hplZ, zCursor + 0.5 * hplZ);
                    CaloFibreHP::buildLayer(env, alMat, psMat, volumeName, 0.0, iHPL,
                                            cfg.plate_xy_mm, cfg.hpl_thickness_mm,
                                            cfg.fiber_diameter_mm, cfg.fiber_core_diameter_mm,
                                            false, moduleTag);
                    zCursor += hplZ;
                    ++iHPL;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::AirGap:
                    zCursor += airGapZ;
                    if (sec.incrementGlobalOnAirGap)
                        ++globalLayerIdx;
                    break;
                default:
                    throw std::runtime_error(
                        std::format("CalorimeterFactory: unknown layer code: {}", code));
            }
        }
    };

    processSection({.prefix = "ecal",
                    .layerCodes = cfg.layers,
                    .absorberLog = leadLog,
                    .absorberHalfZ = 0.5 * leadZ,
                    .absorberNeedsEnvelope = true,
                    .incrementGlobalOnAirGap = false});

    zCursor += cfg.gap_ecal_hcal_mm * mm;

    processSection({.prefix = "hcal",
                    .layerCodes = cfg.layers2,
                    .absorberLog = ironLog,
                    .absorberHalfZ = 0.5 * ironZ,
                    .absorberNeedsEnvelope = false,
                    .incrementGlobalOnAirGap = true});
}

}  // namespace SHiPGeometry
