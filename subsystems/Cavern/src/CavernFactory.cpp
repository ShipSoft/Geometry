// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Cavern/CavernFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

namespace SHiPGeometry {

CavernFactory::CavernFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* CavernFactory::build() {
    // Get materials from central manager with null checking
    auto* air = m_materials.requireMaterial("Air");
    auto* concrete = m_materials.requireMaterial("Concrete");

    // Create world volume (cave)
    auto* worldBox = new GeoBox(s_worldHalfX, s_worldHalfY, s_worldHalfZ);
    auto* worldLog = new GeoLogVol("cave", worldBox, air);
    m_world = new GeoPhysVol(worldLog);

    // Create rock block
    auto* rockBox = new GeoBox(s_rockHalfX, s_rockHalfY, s_rockHalfZ);

    // Create cavities to subtract
    auto* muonCavern = new GeoBox(s_muonCavernHalfX, s_muonCavernHalfY, s_muonCavernHalfZ);
    auto* expCavern = new GeoBox(s_expCavernHalfX, s_expCavernHalfY, s_expCavernHalfZ);
    auto* stairStep = new GeoBox(s_stairHalfX, s_stairHalfY, s_stairHalfZ);
    auto* yokePit = new GeoBox(s_yokePitHalfX, s_yokePitHalfY, s_yokePitHalfZ);
    auto* targetPit = new GeoBox(s_targetPitHalfX, s_targetPitHalfY, s_targetPitHalfZ);

    // Build the cavern shape by successive subtractions
    // First subtract muon shield cavern
    GeoTrf::Transform3D muonCavernTrf =
        GeoTrf::Translate3D(s_muonCavernPosX, s_muonCavernPosY, s_muonCavernPosZ);
    const GeoShape* shape1 = &(rockBox->subtract((*muonCavern) << muonCavernTrf));

    // Subtract experiment cavern
    GeoTrf::Transform3D expCavernTrf =
        GeoTrf::Translate3D(s_expCavernPosX, s_expCavernPosY, s_expCavernPosZ);
    const GeoShape* shape2 = &(shape1->subtract((*expCavern) << expCavernTrf));

    // Subtract stair step
    GeoTrf::Transform3D stairTrf = GeoTrf::Translate3D(s_stairPosX, s_stairPosY, s_stairPosZ);
    const GeoShape* shape3 = &(shape2->subtract((*stairStep) << stairTrf));

    // Subtract yoke pit
    GeoTrf::Transform3D yokePitTrf =
        GeoTrf::Translate3D(s_yokePitPosX, s_yokePitPosY, s_yokePitPosZ);
    const GeoShape* shape4 = &(shape3->subtract((*yokePit) << yokePitTrf));

    // Subtract target pit
    GeoTrf::Transform3D targetPitTrf =
        GeoTrf::Translate3D(s_targetPitPosX, s_targetPitPosY, s_targetPitPosZ);
    const GeoShape* cavernShape = &(shape4->subtract((*targetPit) << targetPitTrf));

    // Create cavern logical and physical volumes
    auto* cavernLog = new GeoLogVol("Cavern", cavernShape, concrete);
    m_cavern = new GeoPhysVol(cavernLog);

    // Place cavern in world with name tag
    GeoTrf::Transform3D cavernTrf = GeoTrf::Translate3D(0.0, 0.0, s_cavernPosZ);
    m_world->add(new GeoNameTag("Cavern"));
    m_world->add(new GeoTransform(cavernTrf));
    m_world->add(m_cavern);

    return m_world;
}

}  // namespace SHiPGeometry
