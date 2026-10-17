// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"

#include "SHiP/geometry/Calorimeter/CaloBarLayer.h"
#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"
#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

#include <format>
#include <span>
#include <string>

namespace SHiP::geometry {

using namespace GeoModelKernelUnits;

// ── constructor ──────────────────────────────────────────────────────────────

CalorimeterFactory::CalorimeterFactory(Materials& materials) : m_materials(materials) {}

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* CalorimeterFactory::build() {
    GeoMaterial* air = m_materials.requireMaterial("Air");

    // Fixed-size container — must match the SHiP subsystem envelope so that
    // the geometry consistency tests and the overlap check pass.
    auto* containerBox = new GeoBox(calo::kContainerHalfX * mm, calo::kContainerHalfY * mm,
                                    calo::kContainerHalfZ * mm);
    auto* containerLog = new GeoLogVol("/SHiP/calorimeter", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    const double x0 = -0.5 * (calo::kModuleNX - 1) * calo::kModulePitchX;
    const double y0 = -0.5 * (calo::kModuleNY - 1) * calo::kModulePitchY;

    for (int iy = 0; iy < calo::kModuleNY; ++iy)
        for (int ix = 0; ix < calo::kModuleNX; ++ix)
            buildStack(containerPhys, ix, iy, x0 + ix * calo::kModulePitchX,
                       y0 + iy * calo::kModulePitchY);

    return containerPhys;
}

// ── buildStack ───────────────────────────────────────────────────────────────

void CalorimeterFactory::buildStack(GeoPhysVol* container, int moduleX, int moduleY, double offsetX,
                                    double offsetY) const {
    using calo::LayerCode;

    GeoMaterial* leadMat = m_materials.requireMaterial("Lead");
    GeoMaterial* ironMat = m_materials.requireMaterial("Iron");
    GeoMaterial* pvtMat = m_materials.requireMaterial("PVT");
    GeoMaterial* psMat = m_materials.requireMaterial("Polystyrene");
    GeoMaterial* alMat = m_materials.requireMaterial("Aluminium");
    GeoMaterial* airMat = m_materials.requireMaterial("Air");

    const double plateXY = calo::kPlateXY * mm;
    const double leadZ = calo::kLeadThickness * mm;
    const double scintZ = calo::kScintThickness * mm;
    const double hplZ = calo::kHplThickness * mm;
    const double ironZ = calo::kIronThickness * mm;
    const double airGapZ = calo::kAirGap * mm;

    const double wideW = calo::kWidePVTBarPitch * mm;
    const double thinW = calo::kThinPSBarPitch * mm;

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

    // z cursor: the layer stack is centred at z=0 in the container volume.
    double zCursor = -0.5 * calo::kTotalStackZ * mm;

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
        std::span<const LayerCode> layerCodes;
        GeoLogVol* absorberLog;
        double absorberHalfZ;
        bool absorberNeedsEnvelope;    // ECAL lead: true, HCAL iron: false
        bool incrementGlobalOnAirGap;  // ECAL: false, HCAL: true
    };

    auto processSection = [&](const SectionDescriptor& sec) {
        int globalLayerIdx = 0, scintLayerIdx = 0;
        int absorberIdx = 0;

        for (LayerCode code : sec.layerCodes) {
            const auto basePath =
                std::format("/SHiP/calorimeter/{}/gl{}", sec.prefix, globalLayerIdx);

            switch (code) {
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
                    calo_bar::placeLayer(env, wideHLog, calo::kWidePVTBarPitch,
                                         calo::kWidePVTBarCount, 0.0, volumeName.c_str(), iWideH,
                                         BarAxis::AlongY, moduleTag);
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
                    calo_bar::placeLayer(env, wideVLog, calo::kWidePVTBarPitch,
                                         calo::kWidePVTBarCount, 0.0, volumeName.c_str(), iWideV,
                                         BarAxis::AlongX, moduleTag);
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
                    calo_bar::placeLayer(env, thinHLog, calo::kThinPSBarPitch,
                                         calo::kThinPSBarCount, 0.0, volumeName.c_str(), iThinH,
                                         BarAxis::AlongY, moduleTag);
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
                    calo_bar::placeLayer(env, thinVLog, calo::kThinPSBarPitch,
                                         calo::kThinPSBarCount, 0.0, volumeName.c_str(), iThinV,
                                         BarAxis::AlongX, moduleTag);
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
                    calo_fibre_hp::buildLayer(env, alMat, psMat, volumeName, 0.0, iHPL,
                                              calo::kPlateXY, calo::kHplThickness,
                                              calo::kFiberDiameter, calo::kFiberCoreDiameter, true,
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
                    calo_fibre_hp::buildLayer(env, alMat, psMat, volumeName, 0.0, iHPL,
                                              calo::kPlateXY, calo::kHplThickness,
                                              calo::kFiberDiameter, calo::kFiberCoreDiameter, false,
                                              moduleTag);
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
            }
        }
    };

    processSection({.prefix = "ecal",
                    .layerCodes = calo::kEcalLayers,
                    .absorberLog = leadLog,
                    .absorberHalfZ = 0.5 * leadZ,
                    .absorberNeedsEnvelope = true,
                    .incrementGlobalOnAirGap = false});

    zCursor += calo::kGapEcalHcal * mm;

    processSection({.prefix = "hcal",
                    .layerCodes = calo::kHcalLayers,
                    .absorberLog = ironLog,
                    .absorberHalfZ = 0.5 * ironZ,
                    .absorberNeedsEnvelope = false,
                    .incrementGlobalOnAirGap = true});
}

}  // namespace SHiP::geometry
