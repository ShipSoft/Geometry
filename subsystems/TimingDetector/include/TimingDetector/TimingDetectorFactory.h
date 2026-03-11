// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the TimingDetector (TimeDet) geometry.
 *
 * Builds a container volume and populates it with 330 scintillator bars
 * (3 columns × 110 rows) via GeoModelXML (Gmx2Geo).  Each bar is a
 * GeoFullPhysVol (sensitive volume) registered through SHiPTimingDetInterface.
 *
 * Container half-sizes (mm): 2750 × 3250 × 250.
 * Bar half-sizes (mm):        700 × 30 × 5.
 */
class TimingDetectorFactory {
   public:
    explicit TimingDetectorFactory(SHiPMaterials& materials);
    ~TimingDetectorFactory() = default;

    /** Build the TimingDetector geometry and return the container volume. */
    GeoPhysVol* build();

    /** Number of sensitive bars registered during the last build() call. */
    int barCount() const { return m_barCount; }

   private:
    SHiPMaterials& m_materials;
    int m_barCount{0};

    // Container dimensions (mm)
    static constexpr double s_containerHalfX = 2750.0;
    static constexpr double s_containerHalfY = 3250.0;
    static constexpr double s_containerHalfZ = 250.0;
};

}  // namespace SHiPGeometry
