// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the DecayVolume (decay vessel and veto system) geometry
 */
class DecayVolumeFactory {
public:
    DecayVolumeFactory() = default;
    ~DecayVolumeFactory() = default;

    /**
     * @brief Build the DecayVolume geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry