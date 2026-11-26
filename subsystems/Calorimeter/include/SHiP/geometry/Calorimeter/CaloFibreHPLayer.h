// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/Units.h"

#include <string>

class GeoVPhysVol;
class GeoPhysVol;
class GeoMaterial;

namespace SHiP::geometry {

/**
 * @brief Hadronic Pre-shower Layers (HPL) of scintillating fibres.
 *
 * A layer is an aluminium casing filled with three sublayers of cylindrical
 * fibres (cladding + core). Fibres run along Y when @p fibresAlongY is true,
 * along X otherwise.
 */
namespace calo_fibre_hp {

/**
 * @brief Builds one casing with its fibres, to be placed any number of times.
 *
 * All layers of one orientation are identical, so the calorimeter builds the
 * casing once and places it in every slot. Fibres inside are named
 * `HPL_{V|H}_S{sublayer}_F{index}` and carry the copy number
 * calo::fibreCopyNumber(sublayer, index); the layer and module are given by
 * the casing's placement. Throws std::runtime_error on invalid dimensions,
 * before creating any volume.
 */
GeoPhysVol* makeLayer(const GeoMaterial* aluminiumMat, const GeoMaterial* fibreMat,
                      units::LengthMm casingXY, units::LengthMm casingZ, units::LengthMm fiberDiam,
                      units::LengthMm fiberCoreDiam, bool fibresAlongY);

/// Places @p casing in @p mother at @p zCenter, named `{layerTag}_HPL_Casing{nameSuffix}`.
void placeLayer(GeoVPhysVol* mother, GeoPhysVol* casing, const std::string& layerTag,
                units::LengthMm zCenter, const std::string& nameSuffix);

/// Builds a casing of its own with makeLayer() and places it with placeLayer().
void buildLayer(GeoVPhysVol* mother, const GeoMaterial* aluminiumMat, const GeoMaterial* fibreMat,
                const std::string& layerTag, units::LengthMm zCenter, units::LengthMm casingXY,
                units::LengthMm casingZ, units::LengthMm fiberDiam, units::LengthMm fiberCoreDiam,
                bool fibresAlongY, const std::string& nameSuffix);

}  // namespace calo_fibre_hp

}  // namespace SHiP::geometry
