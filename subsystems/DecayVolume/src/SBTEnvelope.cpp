// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/SBTEnvelope.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrap.h>

#include <string>

namespace SHiPGeometry::SBT {

using units::gm;

void buildHelium(GeoPhysVol* container, const GeoMaterial* helium) {
    for (std::size_t i = 0; i < kHeliumPieces.size(); ++i) {
        const HeliumPiece& p = kHeliumPieces[i];
        const double dz = 0.5 * gm(p.z_hi - p.z_lo);
        const double zMid = 0.5 * gm(p.z_lo + p.z_hi);
        const double dx1 = gm(p.dx_lo);
        const double dy1 = gm(p.dy_lo);
        const double dx2 = gm(p.dx_hi);
        const double dy2 = gm(p.dy_hi);

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
