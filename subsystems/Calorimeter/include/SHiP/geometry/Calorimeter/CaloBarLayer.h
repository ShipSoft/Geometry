// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/Units.h"

#include <string>
#include <string_view>

class GeoVPhysVol;
class GeoLogVol;
class GeoPhysVol;

namespace SHiP::geometry {

/// Which transverse axis the bars are replicated along.
enum class BarAxis { AlongX, AlongY };

/**
 * @brief Places an array of identical scintillator bars into a mother volume.
 *
 * Bars share a single GeoLogVol (reuse). They are spaced at @p pitch
 * centre-to-centre, centred on the mother origin in the transverse plane,
 * and all placed at @p zCenter along Z (local coordinates).
 */
namespace calo_bar {

void placeLayer(GeoVPhysVol* mother, GeoLogVol const* barLog, units::LengthMm pitch, int nBars,
                units::LengthMm zCenter, std::string_view tagPrefix, int layerIndex, BarAxis axis,
                const std::string& nameSuffix = "");

/**
 * @brief Builds an envelope filled with one layer of bars, to be placed any
 * number of times.
 *
 * All calorimeter bar layers of one type are identical, so the calorimeter
 * builds each type once and places it in every slot. The bars fill
 * @p envLog at @p pitch, centred on its origin; bar @p i is named
 * `{barTag}_B{i}` and carries copy number @p i. The layer and module come
 * from the envelope's placement.
 */
GeoPhysVol* makeLayer(const GeoLogVol* envLog, const GeoLogVol* barLog, units::LengthMm pitch,
                      int nBars, BarAxis axis, std::string_view barTag);

}  // namespace calo_bar

}  // namespace SHiP::geometry
