// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

class GeoVPhysVol;
class GeoMaterial;

namespace SHiPGeometry {

struct SBTConfig;

/**
 * @brief Builds the SBT steel H-beam supporting structure into @p mother.
 *
 * Ports the standalone SBTStructureBuilder: a rectangular frustum of HEA 260
 * H-beams (each modelled as three GeoBox siblings) comprising vertical
 * columns, segmented corner beams, top/bottom longitudinal beams and
 * top/bottom cross-beams. All pieces are placed as direct children of
 * @p mother (a flat architecture the clash-avoidance logic depends on).
 */
class SBTStructureBuilder {
   public:
    /// Build the structure. @p steel is the absorber material; @p cfg holds
    /// the frustum and H-beam parameters; @p tag is the volume-name prefix.
    static void build(GeoVPhysVol* mother, const GeoMaterial* steel, const SBTConfig& cfg,
                      const std::string& tag = "/SHiP/decay_volume/sbt/structure");
};

}  // namespace SHiPGeometry
