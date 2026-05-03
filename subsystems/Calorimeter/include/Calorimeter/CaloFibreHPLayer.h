// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

class GeoVPhysVol;
class GeoMaterial;

namespace SHiPGeometry {

/**
 * @brief Builds one Hadronic Pre-shower Layer (HPL) of scintillating fibres.
 *
 * The layer consists of an aluminium casing filled with three sublayers of
 * cylindrical fibres (cladding + core). Fibres run along Y when
 * @p fibresAlongY is true, along X otherwise.
 */
class CaloFibreHPLayer {
   public:
    static void build(GeoVPhysVol* mother, GeoMaterial* aluminiumMat, GeoMaterial* fibreMat,
                      const std::string& layerTag, double zCenter_mm, int layerIndex,
                      double casingXY_mm, double casingZ_mm, double fiberDiam_mm,
                      double fiberCoreDiam_mm, bool fibresAlongY, const std::string& nameSuffix);
};

}  // namespace SHiPGeometry
