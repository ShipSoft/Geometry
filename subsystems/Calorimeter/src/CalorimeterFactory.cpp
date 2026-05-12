// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiPGeometry {

CalorimeterFactory::CalorimeterFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* CalorimeterFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Create container volume that spans all calorimeter components
    auto* containerBox = new GeoBox(kContainerHalfX, kContainerHalfY, kContainerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/calorimeter", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    for (const auto& [name, id, halfX, halfY, halfZ, centreZ] : kComponents) {
        auto* box = new GeoBox(halfX, halfY, halfZ);
        auto* log = new GeoLogVol(name, box, air);
        auto* phys = new GeoPhysVol(log);

        double relativeZ = centreZ - kContainerCentreZ;
        containerPhys->add(new GeoNameTag(name));
        containerPhys->add(new GeoIdentifierTag(id));
        containerPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, relativeZ)));
        containerPhys->add(phys);
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
