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

#include <fstream>
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

// ── file-scope helper ────────────────────────────────────────────────────────

static std::string resolveTomlPath(const std::string& path) {
    if (!path.empty() && path[0] == '/')
        return path;  // already absolute
    if (std::ifstream(path).good())
        return path;  // found relative to CWD
    const std::string srcFallback = CALO_TOML_DEFAULT_PATH;
    if (std::ifstream(srcFallback).good())
        return srcFallback;  // source-tree fallback
    const std::string installFallback = CALO_TOML_INSTALL_PATH;
    if (!installFallback.empty() && std::ifstream(installFallback).good())
        return installFallback;  // installed data dir
    return path;                 // give up — readCaloConfig will emit the error
}

// ── constructor / accessors ──────────────────────────────────────────────────

CalorimeterFactory::CalorimeterFactory(SHiPMaterials& materials, std::string configPath)
    : m_materials(materials), m_configPath(std::move(configPath)) {}

std::string CalorimeterFactory::resolvedConfigPath() const {
    return resolveTomlPath(m_configPath);
}

// ── totalStackZ ──────────────────────────────────────────────────────────────

double CalorimeterFactory::totalStackZ(const CalorimeterConfig& cfg) {
    double total = 0.0;
    for (int code : cfg.layers) {
        if (code == 7)
            total += cfg.lead_thickness_mm;
        else if (code >= 1 && code <= 4)
            total += cfg.scint_thickness_mm;
        else if (code == 5 || code == 6)
            total += cfg.hpl_thickness_mm;
        else if (code == 8)
            total += cfg.airgap_mm;
        else
            throw std::runtime_error("CalorimeterFactory: unknown layer code in 'layers': " +
                                     std::to_string(code));
    }
    total += cfg.gap_ecal_hcal_mm;
    for (int code : cfg.layers2) {
        if (code == 7)
            total += cfg.iron_thickness_mm;
        else if (code >= 1 && code <= 4)
            total += cfg.scint_thickness_mm;
        else if (code == 5 || code == 6)
            total += cfg.hpl_thickness_mm;
        else if (code == 8)
            total += cfg.airgap_mm;
        else
            throw std::runtime_error("CalorimeterFactory: unknown layer code in 'layers2': " +
                                     std::to_string(code));
    }
    return total;
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
        throw std::runtime_error("CalorimeterFactory: calo.toml total stack Z (" +
                                 std::to_string(stackZ) + " mm) exceeds container half-Z*2 (" +
                                 std::to_string(2.0 * s_containerHalfZ) +
                                 " mm). Reduce layer thicknesses or number of layers.");
    if (maxHalfX > s_containerHalfX)
        throw std::runtime_error("CalorimeterFactory: calo.toml module array half-X (" +
                                 std::to_string(maxHalfX) + " mm) exceeds container half-X (" +
                                 std::to_string(s_containerHalfX) +
                                 " mm). Reduce plate_xy_mm, module_nx, or module_pitch_x_mm.");
    if (maxHalfY > s_containerHalfY)
        throw std::runtime_error("CalorimeterFactory: calo.toml module array half-Y (" +
                                 std::to_string(maxHalfY) + " mm) exceeds container half-Y (" +
                                 std::to_string(s_containerHalfY) +
                                 " mm). Reduce plate_xy_mm, module_ny, or module_pitch_y_mm.");

    const double x0 = -0.5 * (cfg.module_nx - 1) * pitchX;
    const double y0 = -0.5 * (cfg.module_ny - 1) * pitchY;

    for (int iy = 0; iy < cfg.module_ny; ++iy)
        for (int ix = 0; ix < cfg.module_nx; ++ix)
            buildStack(containerPhys, cfg, ix, iy, x0 + ix * pitchX, y0 + iy * pitchY);

    return containerPhys;
}

// ── buildStack ───────────────────────────────────────────────────────────────

void CalorimeterFactory::buildStack(GeoPhysVol* container, const CalorimeterConfig& cfg, int mx,
                                    int my, double offX, double offY) const {
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

    const double wideW = 60.0 * mm;
    const double thinW = 10.0 * mm;

    const std::string mtag = "_MX" + std::to_string(mx) + "Y" + std::to_string(my);

    // Reusable LogVols — GeoModel shares them across GeoPhysVol instances
    auto* leadLog = new GeoLogVol("/SHiP/calorimeter/lead_plate" + mtag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * leadZ), leadMat);
    auto* ironLog = new GeoLogVol("/SHiP/calorimeter/iron_plate" + mtag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * ironZ), ironMat);
    auto* wideHLog = new GeoLogVol("/SHiP/calorimeter/wide_pvt_h" + mtag,
                                   new GeoBox(0.5 * plateXY, 0.5 * wideW, 0.5 * scintZ), pvtMat);
    auto* wideVLog = new GeoLogVol("/SHiP/calorimeter/wide_pvt_v" + mtag,
                                   new GeoBox(0.5 * wideW, 0.5 * plateXY, 0.5 * scintZ), pvtMat);
    auto* thinHLog = new GeoLogVol("/SHiP/calorimeter/thin_ps_h" + mtag,
                                   new GeoBox(0.5 * plateXY, 0.5 * thinW, 0.5 * scintZ), psMat);
    auto* thinVLog = new GeoLogVol("/SHiP/calorimeter/thin_ps_v" + mtag,
                                   new GeoBox(0.5 * thinW, 0.5 * plateXY, 0.5 * scintZ), psMat);

    // z cursor: start at -halfContainerZ if centering, else 0
    // We centre within the fixed container (halfZ = s_containerHalfZ).
    const double totalZ = totalStackZ(cfg);
    double zCursor = cfg.center_stack ? -0.5 * totalZ * mm : -s_containerHalfZ;

    int layerId = 0;  // monotonic identifier used for GeoIdentifierTag

    // Lambda: wrap a layer in a named air-box envelope inside the container
    auto makeEnv = [&](const std::string& name, double halfZ, double zCenter) -> GeoPhysVol* {
        auto* s = new GeoBox(0.5 * plateXY, 0.5 * plateXY, halfZ);
        auto* l = new GeoLogVol(name + "_env", s, airMat);
        auto* p = new GeoPhysVol(l);
        container->add(new GeoNameTag(name.c_str()));
        container->add(new GeoIdentifierTag(layerId++));
        container->add(new GeoTransform(GeoTrf::Translate3D(offX, offY, zCenter)));
        container->add(p);
        return p;
    };

    int iWideH = 0, iWideV = 0, iThinH = 0, iThinV = 0, iHPL = 0;
    int gl = 0, sl = 0;

    // ══════════════════════════════════════════
    // ECAL section  (code 7 → Lead)
    // ══════════════════════════════════════════
    for (int code : cfg.layers) {
        if (code == 7) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_lead" + mtag;
            auto* env = makeEnv(n, 0.5 * leadZ, zCursor + 0.5 * leadZ);
            env->add(new GeoNameTag(n.c_str()));
            env->add(new GeoIdentifierTag(0));
            env->add(new GeoTransform(GeoTrf::Translate3D(0, 0, 0)));
            env->add(new GeoPhysVol(leadLog));
            zCursor += leadZ;
            ++gl;

        } else if (code == 1) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_wide_pvt_h" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, wideHLog, 60.0, 36, 0.0, n.c_str(), iWideH, BarAxis::AlongY,
                                mtag);
            zCursor += scintZ;
            ++iWideH;
            ++gl;
            ++sl;

        } else if (code == 2) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_wide_pvt_v" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, wideVLog, 60.0, 36, 0.0, n.c_str(), iWideV, BarAxis::AlongX,
                                mtag);
            zCursor += scintZ;
            ++iWideV;
            ++gl;
            ++sl;

        } else if (code == 3) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_thin_ps_h" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, thinHLog, 10.0, 216, 0.0, n.c_str(), iThinH, BarAxis::AlongY,
                                mtag);
            zCursor += scintZ;
            ++iThinH;
            ++gl;
            ++sl;

        } else if (code == 4) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_thin_ps_v" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, thinVLog, 10.0, 216, 0.0, n.c_str(), iThinV, BarAxis::AlongX,
                                mtag);
            zCursor += scintZ;
            ++iThinV;
            ++gl;
            ++sl;

        } else if (code == 5) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_hpl_y" + mtag;
            auto* env = makeEnv(n, 0.5 * hplZ, zCursor + 0.5 * hplZ);
            CaloFibreHPLayer::build(env, alMat, psMat, n, 0.0, iHPL, cfg.plate_xy_mm,
                                    cfg.hpl_thickness_mm, cfg.fiber_diameter_mm,
                                    cfg.fiber_core_diameter_mm, true, mtag);
            zCursor += hplZ;
            ++iHPL;
            ++gl;
            ++sl;

        } else if (code == 6) {
            const std::string n = "/SHiP/calorimeter/ecal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_hpl_x" + mtag;
            auto* env = makeEnv(n, 0.5 * hplZ, zCursor + 0.5 * hplZ);
            CaloFibreHPLayer::build(env, alMat, psMat, n, 0.0, iHPL, cfg.plate_xy_mm,
                                    cfg.hpl_thickness_mm, cfg.fiber_diameter_mm,
                                    cfg.fiber_core_diameter_mm, false, mtag);
            zCursor += hplZ;
            ++iHPL;
            ++gl;
            ++sl;

        } else if (code == 8) {
            zCursor += airGapZ;

        } else {
            throw std::runtime_error("CalorimeterFactory: unknown layer code in 'layers': " +
                                     std::to_string(code));
        }
    }

    // ECAL–HCAL gap
    zCursor += cfg.gap_ecal_hcal_mm * mm;

    // ══════════════════════════════════════════
    // HCAL section  (code 7 → Iron)
    // ══════════════════════════════════════════
    int iIron = 0;
    gl = 0;
    sl = 0;

    for (int code : cfg.layers2) {
        if (code == 7) {
            const std::string tag = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_iron_" +
                                    std::to_string(iIron) + mtag;
            container->add(new GeoNameTag(tag.c_str()));
            container->add(new GeoIdentifierTag(layerId++));
            container->add(
                new GeoTransform(GeoTrf::Translate3D(offX, offY, zCursor + 0.5 * ironZ)));
            container->add(new GeoPhysVol(ironLog));
            zCursor += ironZ;
            ++iIron;
            ++gl;

        } else if (code == 1) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_wide_pvt_h" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, wideHLog, 60.0, 36, 0.0, n.c_str(), iWideH, BarAxis::AlongY,
                                mtag);
            zCursor += scintZ;
            ++iWideH;
            ++gl;
            ++sl;

        } else if (code == 2) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_wide_pvt_v" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, wideVLog, 60.0, 36, 0.0, n.c_str(), iWideV, BarAxis::AlongX,
                                mtag);
            zCursor += scintZ;
            ++iWideV;
            ++gl;
            ++sl;

        } else if (code == 3) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_thin_ps_h" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, thinHLog, 10.0, 216, 0.0, n.c_str(), iThinH, BarAxis::AlongY,
                                mtag);
            zCursor += scintZ;
            ++iThinH;
            ++gl;
            ++sl;

        } else if (code == 4) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_thin_ps_v" + mtag;
            auto* env = makeEnv(n, 0.5 * scintZ, zCursor + 0.5 * scintZ);
            CaloBarLayer::place(env, thinVLog, 10.0, 216, 0.0, n.c_str(), iThinV, BarAxis::AlongX,
                                mtag);
            zCursor += scintZ;
            ++iThinV;
            ++gl;
            ++sl;

        } else if (code == 5) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_hpl_y" + mtag;
            auto* env = makeEnv(n, 0.5 * hplZ, zCursor + 0.5 * hplZ);
            CaloFibreHPLayer::build(env, alMat, psMat, n, 0.0, iHPL, cfg.plate_xy_mm,
                                    cfg.hpl_thickness_mm, cfg.fiber_diameter_mm,
                                    cfg.fiber_core_diameter_mm, true, mtag);
            zCursor += hplZ;
            ++iHPL;
            ++gl;
            ++sl;

        } else if (code == 6) {
            const std::string n = "/SHiP/calorimeter/hcal/gl" + std::to_string(gl) + "_sl" +
                                  std::to_string(sl) + "_hpl_x" + mtag;
            auto* env = makeEnv(n, 0.5 * hplZ, zCursor + 0.5 * hplZ);
            CaloFibreHPLayer::build(env, alMat, psMat, n, 0.0, iHPL, cfg.plate_xy_mm,
                                    cfg.hpl_thickness_mm, cfg.fiber_diameter_mm,
                                    cfg.fiber_core_diameter_mm, false, mtag);
            zCursor += hplZ;
            ++iHPL;
            ++gl;
            ++sl;

        } else if (code == 8) {
            zCursor += airGapZ;
            ++gl;

        } else {
            throw std::runtime_error("CalorimeterFactory: unknown layer code in 'layers2': " +
                                     std::to_string(code));
        }
    }
}

}  // namespace SHiPGeometry
