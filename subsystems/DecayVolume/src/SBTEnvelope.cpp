// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/SBTEnvelope.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/Units.h>

#include <string>

namespace SHiPGeometry::SBT {

void buildHelium(GeoPhysVol* container, const GeoMaterial* helium) {
    using namespace GeoModelKernelUnits;

    for (std::size_t i = 0; i < kHeliumPieces.size(); ++i) {
        const HeliumPiece& p = kHeliumPieces[i];
        const double dz = 0.5 * (p.z_hi_mm - p.z_lo_mm) * mm;
        const double zMid = 0.5 * (p.z_lo_mm + p.z_hi_mm) * mm;
        const double dx1 = p.dx_lo_mm * mm;
        const double dy1 = p.dy_lo_mm * mm;
        const double dx2 = p.dx_hi_mm * mm;
        const double dy2 = p.dy_hi_mm * mm;

        const std::string name = "/SHiP/decay_volume/helium_" + std::to_string(i);
        auto* shape = new GeoTrap(dz, 0.0, 0.0, dy1, dx1, dx1, 0.0, dy2, dx2, dx2, 0.0);
        auto* log = new GeoLogVol(name, shape, helium);
        auto* phys = new GeoPhysVol(log);
        container->add(new GeoNameTag(name));
        container->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, zMid)));
        container->add(phys);
    }
}

}  // namespace SHiPGeometry::SBT
