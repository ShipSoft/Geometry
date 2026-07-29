// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/SubsystemDescriptor.h"

#include <GeoModelKernel/Units.h>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the TimingDetector (TimeDet) geometry.
 *
 * Builds an air container populated with 330 scintillator bars arranged in a
 * regular 3-column × 110-row grid (bar positions are fully analytic). The bars
 * share a single GeoLogVol and are placed as GeoPhysVol with hierarchical
 * "/SHiP/timing_detector/bar_<col>_<row>" names; sensitive-detector assignment
 * is performed downstream by name pattern.
 *
 * Container half-sizes (mm): 2750 × 3250 × 250.
 * Bar half-sizes (mm):        700 × 30 × 5.
 * Z stagger (mm):             z = (row%2)·12 + (col%2)·90 → levels 0/12/90/102.
 */
class TimingDetectorFactory {
   public:
    explicit TimingDetectorFactory(SHiPMaterials& materials);
    ~TimingDetectorFactory() = default;

    /// This subsystem's self-description (name, node, id, placement).
    static SubsystemDescriptor descriptor() {
        return {"TimingDetector", "/SHiP/timing_detector", 7, 0.0, 0.0, 95902.0, false};
    }
    /** Build the TimingDetector geometry and return the container volume. */
    [[nodiscard]] GeoPhysVol* build();

    /** Number of bars placed during the last build() call. */
    int barCount() const { return m_barCount; }

   private:
    SHiPMaterials& m_materials;
    int m_barCount{0};

    // Container dimensions
    static constexpr double s_containerHalfX = 2750.0 * GeoModelKernelUnits::mm;
    static constexpr double s_containerHalfY = 3250.0 * GeoModelKernelUnits::mm;
    static constexpr double s_containerHalfZ = 250.0 * GeoModelKernelUnits::mm;

    // Bar dimensions
    static constexpr double s_barHalfX = 700.0 * GeoModelKernelUnits::mm;  // 140 cm full length
    static constexpr double s_barHalfY = 30.0 * GeoModelKernelUnits::mm;   //   6 cm full width
    static constexpr double s_barHalfZ = 5.0 * GeoModelKernelUnits::mm;    //   1 cm full thickness

    // Bar grid layout
    static constexpr int s_nColumns = 3;
    static constexpr int s_nRows = 110;
    static constexpr double s_columnPitchX =
        1300.0 * GeoModelKernelUnits::mm;                                 // column spacing in X
    static constexpr double s_rowY0 = -3220.0 * GeoModelKernelUnits::mm;  // first-row Y
    static constexpr double s_rowStepY =
        6440.0 / 109.0 * GeoModelKernelUnits::mm;  // row pitch ≈ 59.083 mm
    static constexpr double s_zStaggerRow = 12.0 * GeoModelKernelUnits::mm;  // odd-row Z offset
    static constexpr double s_zStaggerCol = 90.0 * GeoModelKernelUnits::mm;  // odd-column Z offset
};

}  // namespace SHiPGeometry
