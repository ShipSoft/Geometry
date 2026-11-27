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
#include <stdexcept>
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

// ── SharedLayers ─────────────────────────────────────────────────────────────

CalorimeterFactory::SharedLayers::Layer CalorimeterFactory::SharedLayers::forCode(
    calo::LayerCode code) const {
    using enum calo::LayerCode;
    const double scintZ = gm(calo::kScintThickness);
    const double hplZ = gm(calo::kHplThickness);
    switch (code) {
        case WidePVT_H:
            return {.type = "wide_pvt_h", .volume = widePvtH, .thickness = scintZ};
        case WidePVT_V:
            return {.type = "wide_pvt_v", .volume = widePvtV, .thickness = scintZ};
        case ThinPS_H:
            return {.type = "thin_ps_h", .volume = thinPsH, .thickness = scintZ};
        case ThinPS_V:
            return {.type = "thin_ps_v", .volume = thinPsV, .thickness = scintZ};
        case FibreHPL_Y:
            return {.type = "hpl_y", .volume = hplAlongY, .thickness = hplZ};
        case FibreHPL_X:
            return {.type = "hpl_x", .volume = hplAlongX, .thickness = hplZ};
        case Absorber:
        case AirGap:
            break;
    }
    throw std::logic_error("CalorimeterFactory: absorbers and air gaps have no shared layer");
}

// ── ModuleStack ──────────────────────────────────────────────────────────────

namespace {

/// A lead plate in an air envelope of its own size, both named @p name.
GeoPhysVol* leadInEnvelope(const std::string& name, const GeoLogVol* leadLog,
                           const GeoMaterial* air, double thickness) {
    const double plateXY = gm(calo::kPlateXY);
    auto* env = new GeoPhysVol(new GeoLogVol(
        name + "_env", new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * thickness), air));
    env->add(new GeoNameTag(name));
    env->add(new GeoIdentifierTag(0));
    env->add(new GeoTransform(GeoTrf::Translate3D(0, 0, 0)));
    env->add(new GeoPhysVol(leadLog));
    return env;
}

}  // namespace

/**
 * Places the layers of one module in the container, front to back, keeping
 * the z cursor and the layer count. Layers are counted from 0 in each module;
 * their copy numbers also encode the module, so they are unique across the
 * calorimeter.
 */
class CalorimeterFactory::ModuleStack {
   public:
    /// One calorimeter section (ECAL or HCAL).
    struct Section {
        std::string_view prefix;  ///< "ecal" or "hcal"
        std::span<const calo::LayerCode> layerCodes;
        const GeoLogVol* absorberLog;
        double absorberZ;              ///< absorber thickness, GeoModel units
        bool absorberNeedsEnvelope;    ///< ECAL lead: true, HCAL iron: false
        bool incrementGlobalOnAirGap;  ///< ECAL: false, HCAL: true
    };

    ModuleStack(GeoPhysVol* container, const SharedLayers& shared, const GeoMaterial* air,
                int moduleX, int moduleY, double offsetX, double offsetY)
        : m_container(container),
          m_shared(&shared),
          m_air(air),
          m_module(calo::moduleIndex(moduleX, moduleY)),
          m_moduleTag(std::format("_MX{}Y{}", moduleX, moduleY)),
          m_offsetX(offsetX),
          m_offsetY(offsetY) {}

    [[nodiscard]] const std::string& moduleTag() const { return m_moduleTag; }

    /// Leaves a gap of @p thickness before the next layer.
    void skip(double thickness) { m_z += thickness; }

    void placeSection(const Section& sec);

   private:
    /// Places @p volume as the next layer, @p thickness deep.
    void place(const std::string& name, GeoVPhysVol* volume, double thickness);

    GeoPhysVol* m_container;
    const SharedLayers* m_shared;
    const GeoMaterial* m_air;
    int m_module;
    std::string m_moduleTag;
    double m_offsetX;
    double m_offsetY;
    double m_z = -0.5 * gm(calo::kTotalStackZ);  // the stack is centred at z = 0
    int m_layerId = 0;
};

void CalorimeterFactory::ModuleStack::place(const std::string& name, GeoVPhysVol* volume,
                                            double thickness) {
    m_container->add(new GeoNameTag(name));
    m_container->add(new GeoIdentifierTag(calo::layerCopyNumber(m_module, m_layerId++)));
    m_container->add(
        new GeoTransform(GeoTrf::Translate3D(m_offsetX, m_offsetY, m_z + (0.5 * thickness))));
    m_container->add(volume);
    m_z += thickness;
}

void CalorimeterFactory::ModuleStack::placeSection(const Section& sec) {
    using enum calo::LayerCode;
    int globalLayerIdx = 0;
    int scintLayerIdx = 0;
    int absorberIdx = 0;

    for (const calo::LayerCode code : sec.layerCodes) {
        const auto basePath = std::format("/SHiP/calorimeter/{}/gl{}", sec.prefix, globalLayerIdx);

        if (code == AirGap) {
            skip(gm(calo::kAirGap));
            if (sec.incrementGlobalOnAirGap) {
                ++globalLayerIdx;
            }
            continue;
        }

        if (code == Absorber) {
            if (sec.absorberNeedsEnvelope) {
                const auto name = std::format("{}_lead{}", basePath, m_moduleTag);
                place(name, leadInEnvelope(name, sec.absorberLog, m_air, sec.absorberZ),
                      sec.absorberZ);
            } else {
                place(std::format("{}_iron_{}{}", basePath, absorberIdx, m_moduleTag),
                      new GeoPhysVol(sec.absorberLog), sec.absorberZ);
            }
            ++absorberIdx;
            ++globalLayerIdx;
            continue;
        }

        // Bar and HPL layers: the shared, filled volume for this type.
        const auto layer = m_shared->forCode(code);
        place(std::format("{}_sl{}_{}{}", basePath, scintLayerIdx, layer.type, m_moduleTag),
              layer.volume, layer.thickness);
        ++globalLayerIdx;
        ++scintLayerIdx;
    }
}

// ── buildStack ───────────────────────────────────────────────────────────────

void CalorimeterFactory::buildStack(GeoPhysVol* container, const SharedLayers& shared, int moduleX,
                                    int moduleY, double offsetX, double offsetY) const {
    const double plateXY = gm(calo::kPlateXY);
    const double leadZ = gm(calo::kLeadThickness);
    const double ironZ = gm(calo::kIronThickness);

    ModuleStack stack(container, shared, m_materials.requireMaterial("Air"), moduleX, moduleY,
                      offsetX, offsetY);

    // One lead and one iron logical volume per module, shared by its plates.
    const auto* leadLog = new GeoLogVol("/SHiP/calorimeter/lead_plate" + stack.moduleTag(),
                                        new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * leadZ),
                                        m_materials.requireMaterial("Lead"));
    const auto* ironLog = new GeoLogVol("/SHiP/calorimeter/iron_plate" + stack.moduleTag(),
                                        new GeoBox(0.5 * plateXY, 0.5 * plateXY, 0.5 * ironZ),
                                        m_materials.requireMaterial("Iron"));

    stack.placeSection({.prefix = "ecal",
                        .layerCodes = calo::kEcalLayers,
                        .absorberLog = leadLog,
                        .absorberZ = leadZ,
                        .absorberNeedsEnvelope = true,
                        .incrementGlobalOnAirGap = false});
    stack.skip(gm(calo::kGapEcalHcal));
    stack.placeSection({.prefix = "hcal",
                        .layerCodes = calo::kHcalLayers,
                        .absorberLog = ironLog,
                        .absorberZ = ironZ,
                        .absorberNeedsEnvelope = false,
                        .incrementGlobalOnAirGap = true});
}

REGISTER_SUBSYSTEM(CalorimeterFactory)

}  // namespace SHiP::geometry
