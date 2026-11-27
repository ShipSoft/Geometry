// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/Units.h"

// GeoModel volumes are reference counted, so their destructors are
// protected on purpose; the check does not apply to a forward declaration.
class GeoPhysVol;  // NOLINT(cppcoreguidelines-virtual-class-destructor)
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

}  // namespace calo_fibre_hp

}  // namespace SHiP::geometry
