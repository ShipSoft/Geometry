// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the MuonShield (muon shield magnets) geometry
 */
class MuonShieldFactory {
public:
    MuonShieldFactory() = default;
    ~MuonShieldFactory() = default;

    /**
     * @brief Build the MuonShield geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry