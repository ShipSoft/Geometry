// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Main geometry builder for the SHiP detector
 */
class SHiPGeometryBuilder {
   public:
    SHiPGeometryBuilder();
    ~SHiPGeometryBuilder();

    /**
     * @brief Build the complete SHiP detector geometry
     * @return Pointer to the world physical volume
     */
    GeoPhysVol* build();
};

}  // namespace SHiPGeometry
