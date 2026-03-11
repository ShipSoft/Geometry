// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiPGeometry {

CalorimeterFactory::CalorimeterFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* CalorimeterFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Create container volume that spans all calorimeter components
    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("CalorimeterContainer", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Create and place ECAL front
    auto* ecalFrontBox = new GeoBox(s_ecalFrontHalfX, s_ecalFrontHalfY, s_ecalFrontHalfZ);
    auto* ecalFrontLog = new GeoLogVol("ECAL_front", ecalFrontBox, air);
    auto* ecalFrontPhys = new GeoPhysVol(ecalFrontLog);

    double ecalFrontRelativeZ = s_ecalFrontZ - s_containerCentreZ;
    GeoTrf::Transform3D ecalFrontTrf = GeoTrf::Translate3D(0.0, 0.0, ecalFrontRelativeZ);

    containerPhys->add(new GeoNameTag("ECAL_front"));
    containerPhys->add(new GeoTransform(ecalFrontTrf));
    containerPhys->add(ecalFrontPhys);

    // Create and place ECAL back
    auto* ecalBackBox = new GeoBox(s_ecalBackHalfX, s_ecalBackHalfY, s_ecalBackHalfZ);
    auto* ecalBackLog = new GeoLogVol("ECAL_back", ecalBackBox, air);
    auto* ecalBackPhys = new GeoPhysVol(ecalBackLog);

    double ecalBackRelativeZ = s_ecalBackZ - s_containerCentreZ;
    GeoTrf::Transform3D ecalBackTrf = GeoTrf::Translate3D(0.0, 0.0, ecalBackRelativeZ);

    containerPhys->add(new GeoNameTag("ECAL_back"));
    containerPhys->add(new GeoTransform(ecalBackTrf));
    containerPhys->add(ecalBackPhys);

    // Create and place HCAL
    auto* hcalBox = new GeoBox(s_hcalHalfX, s_hcalHalfY, s_hcalHalfZ);
    auto* hcalLog = new GeoLogVol("HCAL", hcalBox, air);
    auto* hcalPhys = new GeoPhysVol(hcalLog);

    double hcalRelativeZ = s_hcalZ - s_containerCentreZ;
    GeoTrf::Transform3D hcalTrf = GeoTrf::Translate3D(0.0, 0.0, hcalRelativeZ);

    containerPhys->add(new GeoNameTag("HCAL"));
    containerPhys->add(new GeoTransform(hcalTrf));
    containerPhys->add(hcalPhys);

    return containerPhys;
}

}  // namespace SHiPGeometry
