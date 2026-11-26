// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"

#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>

#include <format>
#include <stdexcept>
#include <string>

namespace SHiP::geometry {

using units::gm;

void calo_fibre_hp::buildLayer(GeoVPhysVol* mother, const GeoMaterial* aluminiumMat,
                               const GeoMaterial* fibreMat, const std::string& layerTag,
                               units::LengthMm zCenter, int layerIndex, units::LengthMm casingXYIn,
                               units::LengthMm casingZIn, units::LengthMm fiberDiam,
                               units::LengthMm fiberCoreDiam, bool fibresAlongY,
                               const std::string& nameSuffix) {
    const double casingXY = gm(casingXYIn);
    const double casingZ = gm(casingZIn);
    const double rOuter = 0.5 * gm(fiberDiam);
    const double rCore = 0.5 * gm(fiberCoreDiam);

    // Validate everything before touching mother, so that a rejected call
    // leaves the caller's tree unchanged.
    if (rCore <= 0.0 || rCore > rOuter) {
        throw std::runtime_error(
            "calo_fibre_hp::buildLayer: invalid core diameter "
            "(must be >0 and <= outer diameter)");
    }
    if (casingZIn < calo::hplMinCasingZ(fiberDiam)) {
        throw std::runtime_error(
            "calo_fibre_hp::buildLayer: casing too thin for three close-packed "
            "fibre sublayers (needs >= fibre diameter * (1 + sqrt(3)))");
    }
    if (calo::fibresPerSublayer(0, casingXYIn, fiberDiam) < 1) {
        throw std::runtime_error(
            "calo_fibre_hp::buildLayer: casing too narrow for a fibre in every "
            "sublayer (needs >= 2 * fibre diameter)");
    }

    // ── aluminium casing ──────────────────────────────────────────────────
    const auto* casingShape = new GeoBox(0.5 * casingXY, 0.5 * casingXY, 0.5 * casingZ);
    const auto* casingLog = new GeoLogVol("HPL_CasingLog", casingShape, aluminiumMat);
    auto* casingPhys = new GeoPhysVol(casingLog);

    mother->add(new GeoNameTag((layerTag + "_HPL_Casing" + nameSuffix).c_str()));
    mother->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, gm(zCenter))));
    mother->add(casingPhys);

    // ── fibre geometry ────────────────────────────────────────────────────
    // GeoTube axis is Z; rotate so fibres run along Y or X
    const double halfLen = 0.5 * casingXY;
    const GeoTrf::Transform3D rotAxis =
        fibresAlongY ? GeoTrf::Transform3D(GeoTrf::RotateX3D(gm(90.0 * units::deg)))    // Z → Y
                     : GeoTrf::Transform3D(GeoTrf::RotateY3D(gm(-90.0 * units::deg)));  // Z → X

    const auto* cladShape = new GeoTube(0.0, rOuter, halfLen);
    const auto* coreShape = new GeoTube(0.0, rCore, halfLen);
    const auto* cladLog = new GeoLogVol("HPL_CladLog", cladShape, fibreMat);
    const auto* coreLog = new GeoLogVol("HPL_FiberCoreLog", coreShape, fibreMat);

    // ── fibre tiling: three hexagonally close-packed sublayers ────────────
    // Positions come from calo::fibreCentre(), shared with the digitiser.
    const std::string orient = fibresAlongY ? "V_L" : "H_L";

    for (int sublayer = 0; sublayer < calo::kHplSublayers; ++sublayer) {
        const int nFib = calo::fibresPerSublayer(sublayer, casingXYIn, fiberDiam);
        for (int i = 0; i < nFib; ++i) {
            const auto centre = calo::fibreCentre(sublayer, i, casingXYIn, fiberDiam);
            const double fibrePosition = gm(centre.across);
            const double sublayerZ = gm(centre.z);

            const auto baseName = std::format("{}_HPL_{}{}_S{}_F{}{}", layerTag, orient, layerIndex,
                                              sublayer, i, nameSuffix);

            // cladding physical volume; core is a child of it
            auto* cladPhys = new GeoPhysVol(cladLog);
            cladPhys->add(new GeoNameTag(std::format("{}_Core", baseName).c_str()));
            cladPhys->add(new GeoPhysVol(coreLog));

            const double xPos = fibresAlongY ? fibrePosition : 0.0;
            const double yPos = fibresAlongY ? 0.0 : fibrePosition;

            casingPhys->add(new GeoNameTag(std::format("{}_Clad", baseName).c_str()));
            casingPhys->add(new GeoTransform(GeoTrf::Translate3D(xPos, yPos, sublayerZ) * rotAxis));
            casingPhys->add(cladPhys);
        }
    }
}

}  // namespace SHiP::geometry
