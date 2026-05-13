// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CaloFibreHPLayer.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/Units.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <string>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

void CaloFibreHP::buildLayer(GeoVPhysVol* mother, GeoMaterial* aluminiumMat, GeoMaterial* fibreMat,
                             const std::string& layerTag, double zCenter_mm, int layerIndex,
                             double casingXY_mm, double casingZ_mm, double fiberDiam_mm,
                             double fiberCoreDiam_mm, bool fibresAlongY,
                             const std::string& nameSuffix) {
    const double casingXY = casingXY_mm * mm;
    const double casingZ = casingZ_mm * mm;

    // ── aluminium casing ──────────────────────────────────────────────────
    auto* casingShape = new GeoBox(0.5 * casingXY, 0.5 * casingXY, 0.5 * casingZ);
    auto* casingLog = new GeoLogVol("HPL_CasingLog", casingShape, aluminiumMat);
    auto* casingPhys = new GeoPhysVol(casingLog);

    mother->add(new GeoNameTag((layerTag + "_HPL_Casing" + nameSuffix).c_str()));
    mother->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, zCenter_mm * mm)));
    mother->add(casingPhys);

    // ── fibre geometry ────────────────────────────────────────────────────
    const double rOuter = 0.5 * fiberDiam_mm * mm;
    const double rCore = 0.5 * fiberCoreDiam_mm * mm;

    if (rCore <= 0.0 || rCore > rOuter)
        throw std::runtime_error(
            "CaloFibreHP::buildLayer: invalid core diameter "
            "(must be >0 and <= outer diameter)");

    // GeoTube axis is Z; rotate so fibres run along Y or X
    const double halfLen = 0.5 * casingXY;
    GeoTrf::Transform3D rotAxis =
        fibresAlongY ? GeoTrf::Transform3D(GeoTrf::RotateX3D(90.0 * deg))    // Z → Y
                     : GeoTrf::Transform3D(GeoTrf::RotateY3D(-90.0 * deg));  // Z → X

    auto* cladShape = new GeoTube(0.0, rOuter, halfLen);
    auto* coreShape = new GeoTube(0.0, rCore, halfLen);
    auto* cladLog = new GeoLogVol("HPL_CladLog", cladShape, fibreMat);
    auto* coreLog = new GeoLogVol("HPL_FiberCoreLog", coreShape, fibreMat);

    // ── fibre tiling: three staggered sublayers ───────────────────────────
    const double pitch = 2.0 * rOuter;  // tight packing
    const double dxMax = 0.5 * pitch;   // middle-sublayer shift
    const double usable = casingXY - 2.0 * rOuter - dxMax;
    const int nFib = std::max(1, static_cast<int>(std::floor(usable / pitch)) + 1);
    const double x0 = -0.5 * ((nFib - 1) * pitch + dxMax);

    const double dx[3] = {0.0, 0.5 * pitch, 0.0};
    const double dz = std::max(0.0, 0.5 * casingZ - rOuter);
    const double zLocal[3] = {-dz, 0.0, +dz};

    const std::string orient = fibresAlongY ? "V_L" : "H_L";

    for (int sublayer = 0; sublayer < 3; ++sublayer) {
        for (int i = 0; i < nFib; ++i) {
            const double fibrePosition = x0 + i * pitch + dx[sublayer];
            const double sublayerZ = zLocal[sublayer];

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

}  // namespace SHiPGeometry
