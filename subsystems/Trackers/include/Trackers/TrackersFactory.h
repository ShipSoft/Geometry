// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the Trackers (straw tube tracking stations) geometry
 */
class TrackersFactory {
public:
    TrackersFactory() = default;
    ~TrackersFactory() = default;

    /**
     * @brief Build the Trackers geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry