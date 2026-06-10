// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

class GeoVPhysVol;
class GeoMaterial;

namespace SHiPGeometry {

struct SBTConfig;

/**
 * @brief Builds the SBT scintillator sensor containers into @p mother.
 *
 * Ports the standalone SBTSensorBuilder: ~130 GeoTrap containers (side and
 * top/bottom faces of the frustum), each holding 7 aluminium walls and 6 LAB
 * cells stacked along Z. Containers are Z-split at the column front-flange
 * edge and inset per-face so that, in the flat (non-hierarchical) layout,
 * neighbouring volumes keep a real air gap.
 */
class SBTSensorBuilder {
   public:
    /// Build the sensors. @p alMat is the aluminium wall material, @p labMat
    /// the LAB cell material; @p cfg holds the frustum/sensor parameters.
    static void build(GeoVPhysVol* mother, const GeoMaterial* alMat, const GeoMaterial* labMat,
                      const SBTConfig& cfg,
                      const std::string& tag = "/SHiP/decay_volume/sbt/sensors");
};

}  // namespace SHiPGeometry
