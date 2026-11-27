// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CaloBarLayer.h"

#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <format>
#include <string>

namespace SHiP::geometry {

using units::gm;

GeoPhysVol* calo_bar::makeLayer(const GeoLogVol* envLog, const GeoLogVol* barLog,
                                units::LengthMm pitch, int nBars, BarAxis axis,
                                std::string_view barTag) {
    auto* envPhys = new GeoPhysVol(envLog);
    const double step = gm(pitch);
    const double firstBarCenter = -0.5 * (nBars - 1) * step;

    for (int i = 0; i < nBars; ++i) {
        const double barCenter = firstBarCenter + (i * step);
        const double x = axis == BarAxis::AlongX ? barCenter : 0.0;
        const double y = axis == BarAxis::AlongX ? 0.0 : barCenter;

        envPhys->add(new GeoNameTag(std::format("{}_B{}", barTag, i)));
        envPhys->add(new GeoIdentifierTag(i));
        envPhys->add(new GeoTransform(GeoTrf::Translate3D(x, y, 0.0)));
        envPhys->add(new GeoPhysVol(barLog));
    }
    return envPhys;
}

}  // namespace SHiP::geometry
