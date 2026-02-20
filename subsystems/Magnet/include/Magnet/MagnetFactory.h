// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the Magnet (spectrometer magnet) geometry
 */
class MagnetFactory {
public:
    MagnetFactory() = default;
    ~MagnetFactory() = default;

    /**
     * @brief Build the Magnet geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry