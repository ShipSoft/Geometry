// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the TimingDetector (timing detector) geometry
 */
class TimingDetectorFactory {
public:
    TimingDetectorFactory() = default;
    ~TimingDetectorFactory() = default;

    /**
     * @brief Build the TimingDetector geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry