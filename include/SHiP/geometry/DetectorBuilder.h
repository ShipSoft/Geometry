// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiP::geometry {

/**
 * @brief Main geometry builder for the SHiP detector
 */
class DetectorBuilder {
   public:
    DetectorBuilder();
    ~DetectorBuilder();

    /**
     * @brief Build the complete SHiP detector geometry
     * @return Pointer to the world physical volume
     */
    [[nodiscard]] GeoPhysVol* build();
};

}  // namespace SHiP::geometry
