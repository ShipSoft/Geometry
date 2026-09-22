// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Cavern/CavernFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>

namespace SHiPGeometry {

using units::gm;

CavernFactory::CavernFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* CavernFactory::build() {
    // Get materials from central manager with null checking
    auto* air = m_materials.requireMaterial("Air");
    auto* concrete = m_materials.requireMaterial("Concrete");

    // Create world volume (cave)
    auto* worldBox = new GeoBox(gm(s_worldHalfX), gm(s_worldHalfY), gm(s_worldHalfZ));
    auto* worldLog = new GeoLogVol("cave", worldBox, air);
    m_world = new GeoPhysVol(worldLog);

    // Create rock block
    auto* rockBox = new GeoBox(gm(s_rockHalfX), gm(s_rockHalfY), gm(s_rockHalfZ));

    // Create cavities to subtract
    auto* muonCavern =
        new GeoBox(gm(s_muonCavernHalfX), gm(s_muonCavernHalfY), gm(s_muonCavernHalfZ));
    auto* expCavern = new GeoBox(gm(s_expCavernHalfX), gm(s_expCavernHalfY), gm(s_expCavernHalfZ));
    auto* stairStep = new GeoBox(gm(s_stairHalfX), gm(s_stairHalfY), gm(s_stairHalfZ));
    auto* yokePit = new GeoBox(gm(s_yokePitHalfX), gm(s_yokePitHalfY), gm(s_yokePitHalfZ));
    auto* targetPit = new GeoBox(gm(s_targetPitHalfX), gm(s_targetPitHalfY), gm(s_targetPitHalfZ));

    // Build the cavern shape by successive subtractions
    // First subtract muon shield cavern
    GeoTrf::Transform3D muonCavernTrf =
        GeoTrf::Translate3D(gm(s_muonCavernPosX), gm(s_muonCavernPosY), gm(s_muonCavernPosZ));
    const GeoShape* shape1 = &(rockBox->subtract((*muonCavern) << muonCavernTrf));

    // Subtract experiment cavern
    GeoTrf::Transform3D expCavernTrf =
        GeoTrf::Translate3D(gm(s_expCavernPosX), gm(s_expCavernPosY), gm(s_expCavernPosZ));
    const GeoShape* shape2 = &(shape1->subtract((*expCavern) << expCavernTrf));

    // Subtract stair step
    GeoTrf::Transform3D stairTrf =
        GeoTrf::Translate3D(gm(s_stairPosX), gm(s_stairPosY), gm(s_stairPosZ));
    const GeoShape* shape3 = &(shape2->subtract((*stairStep) << stairTrf));

    // Subtract yoke pit
    GeoTrf::Transform3D yokePitTrf =
        GeoTrf::Translate3D(gm(s_yokePitPosX), gm(s_yokePitPosY), gm(s_yokePitPosZ));
    const GeoShape* shape4 = &(shape3->subtract((*yokePit) << yokePitTrf));

    // Subtract target pit
    GeoTrf::Transform3D targetPitTrf =
        GeoTrf::Translate3D(gm(s_targetPitPosX), gm(s_targetPitPosY), gm(s_targetPitPosZ));
    const GeoShape* cavernShape = &(shape4->subtract((*targetPit) << targetPitTrf));

    // Create cavern logical and physical volumes
    auto* cavernLog = new GeoLogVol("/SHiP/cavern", cavernShape, concrete);
    m_cavern = new GeoPhysVol(cavernLog);

    // Place cavern in world with name tag
    GeoTrf::Transform3D cavernTrf = GeoTrf::Translate3D(0.0, 0.0, gm(s_cavernPosZ));
    m_world->add(new GeoNameTag("/SHiP/cavern"));
    m_world->add(new GeoIdentifierTag(0));
    m_world->add(new GeoTransform(cavernTrf));
    m_world->add(m_cavern);

    return m_world;
}

}  // namespace SHiPGeometry
