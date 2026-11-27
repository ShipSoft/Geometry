// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"

#include "SHiP/geometry/Calorimeter/CaloBarLayer.h"
#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"
#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <format>
#include <span>
#include <string>

namespace SHiP::geometry {

using units::gm;

// ── constructor ──────────────────────────────────────────────────────────────

CalorimeterFactory::CalorimeterFactory(Materials& materials) : m_materials(materials) {}

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* CalorimeterFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Fixed-size container — must match the SHiP subsystem envelope so that
    // the geometry consistency tests and the overlap check pass.
    auto const* containerBox =
        new GeoBox(gm(calo::kContainerHalfX), gm(calo::kContainerHalfY), gm(calo::kContainerHalfZ));
    const auto* containerLog = new GeoLogVol("/SHiP/calorimeter", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Every scintillator layer of one type is identical, so each type is built
    // once, filled, and placed in every slot of every module.
    const GeoMaterial* alMat = m_materials.requireMaterial("Aluminium");
    const GeoMaterial* psMat = m_materials.requireMaterial("Polystyrene");
    const GeoMaterial* pvtMat = m_materials.requireMaterial("PVT");

    const double plateXY = gm(calo::kPlateXY);
    const double scintZ = gm(calo::kScintThickness);
    const double wideW = gm(calo::kWidePVTBarPitch);
    const double thinW = gm(calo::kThinPSBarPitch);

    const auto* barEnvShape = new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * scintZ);
    const auto barLayer = [&](const std::string& type, const GeoMaterial* mat, double barX,
                              double barY, units::LengthMm pitch, int nBars, BarAxis axis) {
        const auto* barLog = new GeoLogVol("/SHiP/calorimeter/" + type,
                                           new GeoBox(0.5 * barX, 0.5 * barY, 0.5 * scintZ), mat);
        const auto* envLog = new GeoLogVol("/SHiP/calorimeter/" + type + "_env", barEnvShape, air);
        return calo_bar::makeLayer(envLog, barLog, pitch, nBars, axis, type);
    };

    const SharedLayers shared{
        .hplAlongY = calo_fibre_hp::makeLayer(alMat, psMat, calo::kPlateXY, calo::kHplThickness,
                                              calo::kFiberDiameter, calo::kFiberCoreDiameter, true),
        .hplAlongX =
            calo_fibre_hp::makeLayer(alMat, psMat, calo::kPlateXY, calo::kHplThickness,
                                     calo::kFiberDiameter, calo::kFiberCoreDiameter, false),
        .widePvtH = barLayer("wide_pvt_h", pvtMat, plateXY, wideW, calo::kWidePVTBarPitch,
                             calo::kWidePVTBarCount, BarAxis::AlongY),
        .widePvtV = barLayer("wide_pvt_v", pvtMat, wideW, plateXY, calo::kWidePVTBarPitch,
                             calo::kWidePVTBarCount, BarAxis::AlongX),
        .thinPsH = barLayer("thin_ps_h", psMat, plateXY, thinW, calo::kThinPSBarPitch,
                            calo::kThinPSBarCount, BarAxis::AlongY),
        .thinPsV = barLayer("thin_ps_v", psMat, thinW, plateXY, calo::kThinPSBarPitch,
                            calo::kThinPSBarCount, BarAxis::AlongX)};

    const double x0 = -0.5 * (calo::kModuleNX - 1) * gm(calo::kModulePitchX);
    const double y0 = -0.5 * (calo::kModuleNY - 1) * gm(calo::kModulePitchY);

    for (int iy = 0; iy < calo::kModuleNY; ++iy) {
        for (int ix = 0; ix < calo::kModuleNX; ++ix) {
            buildStack(containerPhys, shared, ix, iy, x0 + (ix * gm(calo::kModulePitchX)),
                       y0 + (iy * gm(calo::kModulePitchY)));
        }
    }

    return containerPhys;
}

// ── buildStack ───────────────────────────────────────────────────────────────

void CalorimeterFactory::buildStack(GeoPhysVol* container, const SharedLayers& shared, int moduleX,
                                    int moduleY, double offsetX, double offsetY) const {
    using calo::LayerCode;

    const GeoMaterial* leadMat = m_materials.requireMaterial("Lead");
    const GeoMaterial* ironMat = m_materials.requireMaterial("Iron");
    GeoMaterial* airMat = m_materials.requireMaterial("Air");

    const double plateXY = gm(calo::kPlateXY);
    const double leadZ = gm(calo::kLeadThickness);
    const double scintZ = gm(calo::kScintThickness);
    const double hplZ = gm(calo::kHplThickness);
    const double ironZ = gm(calo::kIronThickness);
    const double airGapZ = gm(calo::kAirGap);

    const std::string moduleTag = std::format("_MX{}Y{}", moduleX, moduleY);

    // Reusable LogVols — GeoModel shares them across GeoPhysVol instances
    auto* leadLog = new GeoLogVol("/SHiP/calorimeter/lead_plate" + moduleTag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * leadZ), leadMat);
    auto* ironLog = new GeoLogVol("/SHiP/calorimeter/iron_plate" + moduleTag,
                                  new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * ironZ), ironMat);

    // z cursor: the layer stack is centred at z=0 in the container volume.
    double zCursor = -0.5 * gm(calo::kTotalStackZ);

    // Layers are counted from 0 in each module; their copy numbers also
    // encode the module, so they are unique across the calorimeter.
    const int module = calo::moduleIndex(moduleX, moduleY);
    int layerId = 0;

    // Lambda: wrap a layer in a named air-box envelope inside the container
    auto makeEnv = [&](const std::string& name, double halfZ, double zCenter) -> GeoPhysVol* {
        const auto* envShape = new GeoBox(0.5 * plateXY, 0.5 * plateXY, halfZ);
        const auto* envLog = new GeoLogVol(name + "_env", envShape, airMat);
        auto* envPhys = new GeoPhysVol(envLog);
        container->add(new GeoNameTag(name.c_str()));
        container->add(new GeoIdentifierTag(calo::layerCopyNumber(module, layerId++)));
        container->add(new GeoTransform(GeoTrf::Translate3D(offsetX, offsetY, zCenter)));
        container->add(envPhys);
        return envPhys;
    };

    // Lambda: place a shared, already filled layer inside the container
    auto placeShared = [&](const std::string& name, GeoPhysVol* layer, double zCenter) {
        container->add(new GeoNameTag(name));
        container->add(new GeoIdentifierTag(calo::layerCopyNumber(module, layerId++)));
        container->add(new GeoTransform(GeoTrf::Translate3D(offsetX, offsetY, zCenter)));
        container->add(layer);
    };

    // Describes one calorimeter section (ECAL or HCAL)
    struct SectionDescriptor {
        std::string_view prefix;  // "ecal" or "hcal"
        std::span<const LayerCode> layerCodes;
        GeoLogVol* absorberLog;
        double absorberHalfZ;
        bool absorberNeedsEnvelope;    // ECAL lead: true, HCAL iron: false
        bool incrementGlobalOnAirGap;  // ECAL: false, HCAL: true
    };

    const auto processSection = [&](const SectionDescriptor& sec) {
        int globalLayerIdx = 0;
        int scintLayerIdx = 0;
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
                        container->add(
                            new GeoIdentifierTag(calo::layerCopyNumber(module, layerId++)));
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
                    placeShared(volumeName, shared.widePvtH, zCursor + (0.5 * scintZ));
                    zCursor += scintZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::WidePVT_V: {
                    const auto volumeName =
                        std::format("{}_sl{}_wide_pvt_v{}", basePath, scintLayerIdx, moduleTag);
                    placeShared(volumeName, shared.widePvtV, zCursor + (0.5 * scintZ));
                    zCursor += scintZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::ThinPS_H: {
                    const auto volumeName =
                        std::format("{}_sl{}_thin_ps_h{}", basePath, scintLayerIdx, moduleTag);
                    placeShared(volumeName, shared.thinPsH, zCursor + (0.5 * scintZ));
                    zCursor += scintZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::ThinPS_V: {
                    const auto volumeName =
                        std::format("{}_sl{}_thin_ps_v{}", basePath, scintLayerIdx, moduleTag);
                    placeShared(volumeName, shared.thinPsV, zCursor + (0.5 * scintZ));
                    zCursor += scintZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::FibreHPL_Y: {
                    const auto volumeName =
                        std::format("{}_sl{}_hpl_y{}", basePath, scintLayerIdx, moduleTag);
                    placeShared(volumeName, shared.hplAlongY, zCursor + (0.5 * hplZ));
                    zCursor += hplZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::FibreHPL_X: {
                    const auto volumeName =
                        std::format("{}_sl{}_hpl_x{}", basePath, scintLayerIdx, moduleTag);
                    placeShared(volumeName, shared.hplAlongX, zCursor + (0.5 * hplZ));
                    zCursor += hplZ;
                    ++globalLayerIdx;
                    ++scintLayerIdx;
                    break;
                }
                case LayerCode::AirGap:
                    zCursor += airGapZ;
                    if (sec.incrementGlobalOnAirGap) {
                        ++globalLayerIdx;
                    }
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

    zCursor += gm(calo::kGapEcalHcal);

    processSection({.prefix = "hcal",
                    .layerCodes = calo::kHcalLayers,
                    .absorberLog = ironLog,
                    .absorberHalfZ = 0.5 * ironZ,
                    .absorberNeedsEnvelope = false,
                    .incrementGlobalOnAirGap = true});
}

REGISTER_SUBSYSTEM(CalorimeterFactory)

}  // namespace SHiP::geometry
