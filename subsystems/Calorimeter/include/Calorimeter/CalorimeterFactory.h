// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the Calorimeter (electromagnetic calorimeter) geometry
 */
class CalorimeterFactory {
public:
    CalorimeterFactory() = default;
    ~CalorimeterFactory() = default;

    /**
     * @brief Build the Calorimeter geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry