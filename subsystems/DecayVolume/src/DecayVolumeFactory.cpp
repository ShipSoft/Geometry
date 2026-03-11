// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/DecayVolumeFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiPGeometry {

DecayVolumeFactory::DecayVolumeFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* DecayVolumeFactory::build() {
    const GeoMaterial* aluminium = m_materials.requireMaterial("Aluminium");
    const GeoMaterial* helium = m_materials.requireMaterial("PressurisedHe90");

    // Outer aluminium vessel
    auto* outerBox = new GeoBox(s_halfX, s_halfY, s_halfZ);
    auto* outerLog = new GeoLogVol("DecayVolume", outerBox, aluminium);
    auto* outerPhys = new GeoPhysVol(outerLog);

    // Inner helium atmosphere
    auto* innerBox = new GeoBox(s_innerHalfX, s_innerHalfY, s_innerHalfZ);
    auto* innerLog = new GeoLogVol("DecayVacuum", innerBox, helium);
    auto* innerPhys = new GeoPhysVol(innerLog);

    // Place inner at centre of outer
    outerPhys->add(new GeoNameTag("DecayVacuum"));
    outerPhys->add(new GeoTransform(GeoTrf::TranslateZ3D(0.0)));
    outerPhys->add(innerPhys);

    return outerPhys;
}

}  // namespace SHiPGeometry
