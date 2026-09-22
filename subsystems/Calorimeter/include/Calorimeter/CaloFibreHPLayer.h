// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

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
namespace CaloFibreHP {

void buildLayer(GeoVPhysVol* mother, GeoMaterial* aluminiumMat, GeoMaterial* fibreMat,
                const std::string& layerTag, units::LengthMm zCenter, int layerIndex,
                units::LengthMm casingXY, units::LengthMm casingZ, units::LengthMm fiberDiam,
                units::LengthMm fiberCoreDiam, bool fibresAlongY, const std::string& nameSuffix);

}  // namespace CaloFibreHP

}  // namespace SHiPGeometry
