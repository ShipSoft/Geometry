// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

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
 * Bars share a single GeoLogVol (reuse). They are spaced at @p pitch_mm
 * centre-to-centre, centred on the mother origin in the transverse plane,
 * and all placed at @p zCenter_mm along Z (local coordinates).
 */
namespace CaloBar {

void placeLayer(GeoVPhysVol* mother, GeoLogVol* barLog, double pitch_mm, int nBars,
                double zCenter_mm, std::string_view tagPrefix, int layerIndex, BarAxis axis,
                const std::string& nameSuffix = "");

}  // namespace CaloBar

}  // namespace SHiPGeometry
