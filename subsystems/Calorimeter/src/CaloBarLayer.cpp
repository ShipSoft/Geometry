// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CaloBarLayer.h"

#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

#include <string>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

void CaloBarLayer::place(GeoVPhysVol* mother, GeoLogVol* barLog, double pitch_mm, int nBars,
                         double zCenter_mm, const char* tagPrefix, int layerIndex, BarAxis axis,
                         const std::string& nameSuffix) {
    const double pitch = pitch_mm * mm;
    const double s0 = -0.5 * (nBars - 1) * pitch;

    for (int i = 0; i < nBars; ++i) {
        const double s = s0 + i * pitch;

        double x = 0.0, y = 0.0;
        if (axis == BarAxis::AlongX)
            x = s;
        else
            y = s;

        const std::string name = std::string(tagPrefix) + "_L" + std::to_string(layerIndex) + "_B" +
                                 std::to_string(i) + nameSuffix;

        auto* barPhys = new GeoPhysVol(barLog);
        mother->add(new GeoNameTag(name.c_str()));
        mother->add(new GeoTransform(GeoTrf::Translate3D(x, y, zCenter_mm * mm)));
        mother->add(barPhys);
    }
}

}  // namespace SHiPGeometry
