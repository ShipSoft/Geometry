// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CaloBarLayer.h"

#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <format>
#include <string>

namespace SHiPGeometry {

using units::gm;

void CaloBar::placeLayer(GeoVPhysVol* mother, GeoLogVol* barLog, units::LengthMm pitch, int nBars,
                         units::LengthMm zCenter, std::string_view tagPrefix, int layerIndex,
                         BarAxis axis, const std::string& nameSuffix) {
    const double step = gm(pitch);
    const double firstBarCenter = -0.5 * (nBars - 1) * step;

    for (int i = 0; i < nBars; ++i) {
        const double barCenter = firstBarCenter + i * step;

        double x = 0.0, y = 0.0;
        if (axis == BarAxis::AlongX)
            x = barCenter;
        else
            y = barCenter;

        const auto name = std::format("{}_L{}_B{}{}", tagPrefix, layerIndex, i, nameSuffix);

        auto* barPhys = new GeoPhysVol(barLog);
        mother->add(new GeoNameTag(name.c_str()));
        mother->add(new GeoTransform(GeoTrf::Translate3D(x, y, gm(zCenter))));
        mother->add(barPhys);
    }
}

}  // namespace SHiPGeometry
