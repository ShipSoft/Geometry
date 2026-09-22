// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

#include <string>
#include <string_view>

class GeoVPhysVol;
class GeoLogVol;

namespace SHiPGeometry {

/// Which transverse axis the bars are replicated along.
enum class BarAxis { AlongX, AlongY };

/**
 * @brief Places an array of identical scintillator bars into a mother volume.
 *
 * Bars share a single GeoLogVol (reuse). They are spaced at @p pitch
 * centre-to-centre, centred on the mother origin in the transverse plane,
 * and all placed at @p zCenter along Z (local coordinates).
 */
namespace CaloBar {

void placeLayer(GeoVPhysVol* mother, GeoLogVol* barLog, units::LengthMm pitch, int nBars,
                units::LengthMm zCenter, std::string_view tagPrefix, int layerIndex, BarAxis axis,
                const std::string& nameSuffix = "");

}  // namespace CaloBar

}  // namespace SHiPGeometry
