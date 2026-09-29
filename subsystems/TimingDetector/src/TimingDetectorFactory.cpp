// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "TimingDetector/TimingDetectorFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <string>

namespace SHiPGeometry {

TimingDetectorFactory::TimingDetectorFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TimingDetectorFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* scint = m_materials.requireMaterial("TimDetScint");

    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/timing_detector", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // One reusable bar logvol, shared across all placements (the GeoModel idiom
    // used by the calorimeter bar layers and the upstream-tagger tiles).
    auto* barLog = new GeoLogVol("/SHiP/timing_detector/bar",
                                 new GeoBox(s_barHalfX, s_barHalfY, s_barHalfZ), scint);

    // 3 columns × 110 rows = 330 bars. Positions are analytic:
    //   x = (ic - 1) * pitch          → -1300, 0, +1300 mm
    //   y = y0 + ir * step            → -3220 … +3220 mm (step 6440/109)
    //   z = (ir%2)*12 + (ic%2)*90     → 4 stagger levels: 0, 12, 90, 102 mm
    m_barCount = 0;
    for (int ic = 0; ic < s_nColumns; ++ic) {
        const double x = (ic - 1) * s_columnPitchX;
        for (int ir = 0; ir < s_nRows; ++ir) {
            const double y = s_rowY0 + ir * s_rowStepY;
            const double z = (ir % 2) * s_zStaggerRow + (ic % 2) * s_zStaggerCol;
            const std::string name =
                "/SHiP/timing_detector/bar_" + std::to_string(ic) + "_" + std::to_string(ir);
            containerPhys->add(new GeoNameTag(name));
            containerPhys->add(new GeoIdentifierTag(m_barCount));
            containerPhys->add(new GeoTransform(GeoTrf::Translate3D(x, y, z)));
            containerPhys->add(new GeoPhysVol(barLog));
            ++m_barCount;
        }
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
