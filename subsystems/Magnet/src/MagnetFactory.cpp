// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Magnet/MagnetFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiPGeometry {

MagnetFactory::MagnetFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* MagnetFactory::build() {
    auto* air = m_materials.requireMaterial("Air");

    // Create container
    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/magnet", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Create and place yoke
    auto* yoke = createYoke();
    containerPhys->add(new GeoNameTag("/SHiP/magnet/yoke"));
    containerPhys->add(new GeoIdentifierTag(0));
    containerPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    containerPhys->add(yoke);

    // Create and place coils
    auto* coil1 = createCoil("/SHiP/magnet/coil_1");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_1"));
    containerPhys->add(new GeoIdentifierTag(1));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(s_coilXOffset, s_coilYOffset, 0.0)));
    containerPhys->add(coil1);

    auto* coil2 = createCoil("/SHiP/magnet/coil_2");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_2"));
    containerPhys->add(new GeoIdentifierTag(2));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(-s_coilXOffset, s_coilYOffset, 0.0)));
    containerPhys->add(coil2);

    auto* coil3 = createCoil("/SHiP/magnet/coil_3");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_3"));
    containerPhys->add(new GeoIdentifierTag(3));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(s_coilXOffset, -s_coilYOffset, 0.0)));
    containerPhys->add(coil3);

    auto* coil4 = createCoil("/SHiP/magnet/coil_4");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/coil_4"));
    containerPhys->add(new GeoIdentifierTag(4));
    containerPhys->add(new GeoTransform(GeoTrf::Translate3D(-s_coilXOffset, -s_coilYOffset, 0.0)));
    containerPhys->add(coil4);

    // Create and place vertical connectors
    auto* cv1 = createVerticalConnector("/SHiP/magnet/connector_1");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/connector_1"));
    containerPhys->add(new GeoIdentifierTag(5));
    containerPhys->add(
        new GeoTransform(GeoTrf::Translate3D(s_connectorXOffset, 0.0, -s_connectorZOffset)));
    containerPhys->add(cv1);

    auto* cv2 = createVerticalConnector("/SHiP/magnet/connector_2");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/connector_2"));
    containerPhys->add(new GeoIdentifierTag(6));
    containerPhys->add(
        new GeoTransform(GeoTrf::Translate3D(-s_connectorXOffset, 0.0, -s_connectorZOffset)));
    containerPhys->add(cv2);

    auto* cv3 = createVerticalConnector("/SHiP/magnet/connector_3");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/connector_3"));
    containerPhys->add(new GeoIdentifierTag(7));
    containerPhys->add(
        new GeoTransform(GeoTrf::Translate3D(s_connectorXOffset, 0.0, s_connectorZOffset)));
    containerPhys->add(cv3);

    auto* cv4 = createVerticalConnector("/SHiP/magnet/connector_4");
    containerPhys->add(new GeoNameTag("/SHiP/magnet/connector_4"));
    containerPhys->add(new GeoIdentifierTag(8));
    containerPhys->add(
        new GeoTransform(GeoTrf::Translate3D(-s_connectorXOffset, 0.0, s_connectorZOffset)));
    containerPhys->add(cv4);

    return containerPhys;
}

GeoPhysVol* MagnetFactory::createYoke() {
    auto* iron = m_materials.requireMaterial("Iron");

    // Outer box
    auto* outerBox = new GeoBox(s_yokeOuterHalfX, s_yokeOuterHalfY, s_yokeOuterHalfZ);
    // Inner cutout
    auto* innerBox = new GeoBox(s_yokeInnerHalfX, s_yokeInnerHalfY, s_yokeInnerHalfZ);
    // Subtract to create yoke shape
    const GeoShape* yokeShape = &(outerBox->subtract(*innerBox));

    auto* yokeLog = new GeoLogVol("/SHiP/magnet/yoke", yokeShape, iron);
    return new GeoPhysVol(yokeLog);
}

GeoPhysVol* MagnetFactory::createCoil(const std::string& name) {
    auto* aluminium = m_materials.requireMaterial("Aluminium");

    auto* coilBox = new GeoBox(s_coilHalfX, s_coilHalfY, s_coilHalfZ);
    auto* coilLog = new GeoLogVol(name, coilBox, aluminium);
    return new GeoPhysVol(coilLog);
}

GeoPhysVol* MagnetFactory::createVerticalConnector(const std::string& name) {
    auto* aluminium = m_materials.requireMaterial("Aluminium");

    auto* connectorBox = new GeoBox(s_connectorHalfX, s_connectorHalfY, s_connectorHalfZ);
    auto* connectorLog = new GeoLogVol(name, connectorBox, aluminium);
    return new GeoPhysVol(connectorLog);
}

}  // namespace SHiPGeometry
