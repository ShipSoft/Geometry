// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"

#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoIntrusivePtr.h>
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

GeoPhysVol* calo_fibre_hp::makeLayer(const GeoMaterial* aluminiumMat, const GeoMaterial* fibreMat,
                                     units::LengthMm casingXYIn, units::LengthMm casingZIn,
                                     units::LengthMm fiberDiam, units::LengthMm fiberCoreDiam,
                                     bool fibresAlongY) {
    const double casingXY = gm(casingXYIn);
    const double casingZ = gm(casingZIn);
    const double rOuter = 0.5 * gm(fiberDiam);
    const double rCore = 0.5 * gm(fiberCoreDiam);

    // Validate everything before creating any volume, so that a rejected
    // call leaks nothing and leaves the caller's tree unchanged.
    if (rCore <= 0.0 || rCore > rOuter) {
        throw std::runtime_error(
            "calo_fibre_hp::makeLayer: invalid core diameter "
            "(must be >0 and <= outer diameter)");
    }
    if (casingZIn < calo::hplMinCasingZ(fiberDiam)) {
        throw std::runtime_error(
            "calo_fibre_hp::makeLayer: casing too thin for three close-packed "
            "fibre sublayers (needs >= fibre diameter * (1 + sqrt(3)))");
    }
    if (calo::fibresPerSublayer(0, casingXYIn, fiberDiam) < 1) {
        throw std::runtime_error(
            "calo_fibre_hp::makeLayer: casing too narrow for a fibre in every "
            "sublayer (needs >= 2 * fibre diameter)");
    }

    const std::string orient = fibresAlongY ? "V" : "H";

    // ── aluminium casing ──────────────────────────────────────────────────
    const auto* casingShape = new GeoBox(0.5 * casingXY, 0.5 * casingXY, 0.5 * casingZ);
    const auto* casingLog = new GeoLogVol("HPL_CasingLog", casingShape, aluminiumMat);
    auto* casingPhys = new GeoPhysVol(casingLog);

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

    // All fibres are identical, so one cladding with its core is placed at
    // every position. The intrusive pointer releases it even if no fibre
    // ends up using it.
    const GeoIntrusivePtr<GeoPhysVol> fibre{new GeoPhysVol(cladLog)};
    fibre->add(new GeoNameTag("HPL_Core"));
    fibre->add(new GeoPhysVol(coreLog));

    // ── fibre tiling: three hexagonally close-packed sublayers ────────────
    // Positions come from calo::fibreCentre(), shared with the digitiser.
    // The casing is shared by every layer of this orientation, so names and
    // copy numbers identify a fibre within the casing only; the layer and
    // module come from the casing's placement.
    for (int sublayer = 0; sublayer < calo::kHplSublayers; ++sublayer) {
        const int nFib = calo::fibresPerSublayer(sublayer, casingXYIn, fiberDiam);
        for (int i = 0; i < nFib; ++i) {
            const auto centre = calo::fibreCentre(sublayer, i, casingXYIn, fiberDiam);
            const double fibrePosition = gm(centre.across);
            const double xPos = fibresAlongY ? fibrePosition : 0.0;
            const double yPos = fibresAlongY ? 0.0 : fibrePosition;
            casingPhys->add(
                new GeoNameTag(std::format("HPL_{}_S{}_F{}_Clad", orient, sublayer, i)));
            casingPhys->add(new GeoIdentifierTag(calo::fibreCopyNumber(sublayer, i)));
            casingPhys->add(
                new GeoTransform(GeoTrf::Translate3D(xPos, yPos, gm(centre.z)) * rotAxis));
            casingPhys->add(fibre);
        }
    }
    return casingPhys;
}

}  // namespace SHiP::geometry
