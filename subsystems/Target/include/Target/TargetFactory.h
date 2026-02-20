// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the Target (proton target and shielding) geometry
 */
class TargetFactory {
public:
    TargetFactory() = default;
    ~TargetFactory() = default;

    /**
     * @brief Build the Target geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry