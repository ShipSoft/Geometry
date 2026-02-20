// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the Cavern (experimental hall) geometry
 */
class CavernFactory {
public:
    CavernFactory() = default;
    ~CavernFactory() = default;

    /**
     * @brief Build the Cavern geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry