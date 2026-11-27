// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Magnet/MagnetFactory.h"

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiP::geometry {

using units::gm;

MagnetFactory::MagnetFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* MagnetFactory::build() {
    const auto* air = m_materials.requireMaterial("Air");

    // Create container
    auto const* containerBox =
        new GeoBox(gm(s_containerHalfX), gm(s_containerHalfY), gm(s_containerHalfZ));
    const auto* containerLog = new GeoLogVol("/SHiP/magnet", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Create and place yoke
    auto* yoke = createYoke();
    containerPhys->add(new GeoNameTag("/SHiP/magnet/yoke"));
    containerPhys->add(new GeoIdentifierTag(0));
    containerPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    containerPhys->add(yoke);

    // Create and place the two vertical coil packs
    auto* coil1 = createCoil("/SHiP/magnet/coil_1");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_1"));
    containerPhys->add(new GeoIdentifierTag(1));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(gm(s_coilXOffset), 0.0, 0.0)));
    containerPhys->add(coil1);

    auto* coil2 = createCoil("/SHiP/magnet/coil_2");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_2"));
    containerPhys->add(new GeoIdentifierTag(2));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(gm(-s_coilXOffset), 0.0, 0.0)));
    containerPhys->add(coil2);

    return containerPhys;
}

GeoPhysVol* MagnetFactory::createYoke() {
    const auto* iron = m_materials.requireMaterial("Iron");

    // Outer box
    auto const* outerBox =
        new GeoBox(gm(s_yokeOuterHalfX), gm(s_yokeOuterHalfY), gm(s_yokeOuterHalfZ));
    // Inner cutout
    auto const* innerBox =
        new GeoBox(gm(s_yokeInnerHalfX), gm(s_yokeInnerHalfY), gm(s_yokeInnerHalfZ));
    // Subtract to create yoke shape
    const GeoShape* yokeShape = &outerBox->subtract(*innerBox);

    const auto* yokeLog = new GeoLogVol("/SHiP/magnet/yoke", yokeShape, iron);
    return new GeoPhysVol(yokeLog);
}

GeoPhysVol* MagnetFactory::createCoil(const std::string& name) {
    const auto* aluminium = m_materials.requireMaterial("Aluminium");

    auto const* coilBox = new GeoBox(gm(s_coilHalfX), gm(s_coilHalfY), gm(s_coilHalfZ));
    const auto* coilLog = new GeoLogVol(name, coilBox, aluminium);
    return new GeoPhysVol(coilLog);
}

REGISTER_SUBSYSTEM(MagnetFactory)

}  // namespace SHiP::geometry
