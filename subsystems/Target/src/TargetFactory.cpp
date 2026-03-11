// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Target/TargetFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>

#include <string>

namespace SHiPGeometry {

TargetFactory::TargetFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TargetFactory::build() {
    // Get materials with null checking
    const GeoMaterial* vacuum = m_materials.requireMaterial("Vacuum");

    // Create target_vacuum_box (main container)
    auto* vacuumBox = new GeoBox(s_vacuumBoxHalfX, s_vacuumBoxHalfY, s_vacuumBoxHalfZ);
    auto* vacuumBoxLog = new GeoLogVol("target_vacuum_box", vacuumBox, vacuum);
    auto* vacuumBoxPhys = new GeoPhysVol(vacuumBoxLog);

    // Create and place proximity shielding
    auto* proximityShielding = createProximityShielding();
    GeoTrf::Transform3D proxTrf = GeoTrf::Translate3D(0.0, s_proxPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("proximity_shielding"));
    vacuumBoxPhys->add(new GeoTransform(proxTrf));
    vacuumBoxPhys->add(proximityShielding);

    // Create and place top shielding
    auto* topShielding = createTopShielding();
    GeoTrf::Transform3D topTrf = GeoTrf::Translate3D(0.0, s_topShieldPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("top_shielding"));
    vacuumBoxPhys->add(new GeoTransform(topTrf));
    vacuumBoxPhys->add(topShielding);

    // Create and place bottom shielding
    auto* bottomShielding = createBottomShielding();
    GeoTrf::Transform3D bottomTrf = GeoTrf::Translate3D(0.0, s_bottomShieldPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("bottom_shielding"));
    vacuumBoxPhys->add(new GeoTransform(bottomTrf));
    vacuumBoxPhys->add(bottomShielding);

    // Create and place shielding pedestal
    auto* shieldingPedestal = createShieldingPedestal();
    GeoTrf::Transform3D pedestalTrf = GeoTrf::Translate3D(0.0, s_pedestalPosY, s_pedestalPosZ);
    vacuumBoxPhys->add(new GeoNameTag("shielding_pedestal"));
    vacuumBoxPhys->add(new GeoTransform(pedestalTrf));
    vacuumBoxPhys->add(shieldingPedestal);

    // Place vessel components directly in vacuum box with corrected positions
    // Original TargetArea position: (0, 14.45, -43.25) cm
    // Component positions are relative to TargetArea, so add the offset

    // Create and place target vessel
    // Original: (0, 0, 79.32) in TargetArea → (0, 14.45, 36.07) in vacuum_box
    auto* targetVessel = createTargetVessel();
    GeoTrf::Transform3D vesselTrf =
        GeoTrf::Translate3D(0.0, s_targetAreaPosY, s_targetAreaPosZ + s_vesselPosZ);
    vacuumBoxPhys->add(new GeoNameTag("TargetVessel"));
    vacuumBoxPhys->add(new GeoTransform(vesselTrf));
    vacuumBoxPhys->add(targetVessel);

    // Create and place target vessel front cap
    // Original: (0, 0, -6.6) in TargetArea → (0, 14.45, -49.85) in vacuum_box
    auto* targetVesselFront = createTargetVesselFront();
    GeoTrf::Transform3D vesselFrontTrf =
        GeoTrf::Translate3D(0.0, s_targetAreaPosY, s_targetAreaPosZ + s_vesselFrontPosZ);
    vacuumBoxPhys->add(new GeoNameTag("TargetVesselFront"));
    vacuumBoxPhys->add(new GeoTransform(vesselFrontTrf));
    vacuumBoxPhys->add(targetVesselFront);

    // Create and place target vessel back cap
    // Original: (0, 0, 165.24) in TargetArea → (0, 14.45, 121.99) in vacuum_box
    auto* targetVesselBack = createTargetVesselBack();
    GeoTrf::Transform3D vesselBackTrf =
        GeoTrf::Translate3D(0.0, s_targetAreaPosY, s_targetAreaPosZ + s_vesselBackPosZ);
    vacuumBoxPhys->add(new GeoNameTag("TargetVesselBack"));
    vacuumBoxPhys->add(new GeoTransform(vesselBackTrf));
    vacuumBoxPhys->add(targetVesselBack);

    // Create and place HeVolume (contains target enclosure and slabs)
    // Original: (0, 0, 79.32) in TargetArea → (0, 14.45, 36.07) in vacuum_box
    auto* heVolume = createHeVolume();
    GeoTrf::Transform3D heVolumeTrf =
        GeoTrf::Translate3D(0.0, s_targetAreaPosY, s_targetAreaPosZ + s_heVolumePosZ);
    vacuumBoxPhys->add(new GeoNameTag("HeVolume"));
    vacuumBoxPhys->add(new GeoTransform(heVolumeTrf));
    vacuumBoxPhys->add(heVolume);

    return vacuumBoxPhys;
}

GeoPhysVol* TargetFactory::createProximityShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    // Envelope box
    auto* envelope = new GeoBox(s_proxEnvHalfX, s_proxEnvHalfY, s_proxEnvHalfZ);
    // Inner cutout box
    auto* inner = new GeoBox(s_proxInnerHalfX, s_proxInnerHalfY, s_proxInnerHalfZ);
    // Beam hole
    auto* hole = new GeoTube(0.0, s_proxHoleRadius, s_proxHoleHalfZ);

    // First subtraction: envelope - inner
    GeoTrf::Transform3D innerTrf = GeoTrf::Translate3D(0.0, 0.0, s_proxInnerOffsetZ);
    const GeoShape* shape1 = &(envelope->subtract((*inner) << innerTrf));

    // Second subtraction: shape1 - hole
    GeoTrf::Transform3D holeTrf = GeoTrf::Translate3D(0.0, s_proxHoleOffsetY, s_proxHoleOffsetZ);
    const GeoShape* proxShape = &(shape1->subtract((*hole) << holeTrf));

    auto* proxLog = new GeoLogVol("proximity_shielding", proxShape, copper);
    return new GeoPhysVol(proxLog);
}

GeoPhysVol* TargetFactory::createTopShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* topBox = new GeoBox(s_topShieldHalfX, s_topShieldHalfY, s_topShieldHalfZ);
    auto* topLog = new GeoLogVol("top_shielding", topBox, copper);
    return new GeoPhysVol(topLog);
}

GeoPhysVol* TargetFactory::createBottomShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* bottomBox = new GeoBox(s_bottomShieldHalfX, s_bottomShieldHalfY, s_bottomShieldHalfZ);
    auto* bottomLog = new GeoLogVol("bottom_shielding", bottomBox, copper);
    return new GeoPhysVol(bottomLog);
}

GeoPhysVol* TargetFactory::createShieldingPedestal() {
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto* pedestalBox = new GeoBox(s_pedestalHalfX, s_pedestalHalfY, s_pedestalHalfZ);
    auto* pedestalLog = new GeoLogVol("shielding_pedestal", pedestalBox, iron);
    return new GeoPhysVol(pedestalLog);
}

GeoPhysVol* TargetFactory::createTargetVessel() {
    const GeoMaterial* inconel718 = m_materials.requireMaterial("Inconel718");

    auto* vesselTube = new GeoTube(s_vesselRmin, s_vesselRmax, s_vesselHalfZ);
    auto* vesselLog = new GeoLogVol("TargetVessel", vesselTube, inconel718);
    return new GeoPhysVol(vesselLog);
}

GeoPhysVol* TargetFactory::createTargetVesselFront() {
    const GeoMaterial* inconel718 = m_materials.requireMaterial("Inconel718");

    auto* frontDisk = new GeoTube(0.0, s_vesselCapRadius, s_vesselCapHalfZ);
    auto* frontLog = new GeoLogVol("TargetVesselFront", frontDisk, inconel718);
    return new GeoPhysVol(frontLog);
}

GeoPhysVol* TargetFactory::createTargetVesselBack() {
    const GeoMaterial* inconel718 = m_materials.requireMaterial("Inconel718");

    auto* backDisk = new GeoTube(0.0, s_vesselCapRadius, s_vesselCapHalfZ);
    auto* backLog = new GeoLogVol("TargetVesselBack", backDisk, inconel718);
    return new GeoPhysVol(backLog);
}

GeoPhysVol* TargetFactory::createTargetEnclosure() {
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");

    auto* outerTube = new GeoTube(s_enclosureRmin, s_enclosureRmax, s_enclosureHalfZ);
    auto* cutoutBox = new GeoBox(s_enclosureCutoutHalfX, s_enclosureCutoutHalfY, s_enclosureHalfZ);
    const GeoShape* enclosureShape = &(outerTube->subtract(*cutoutBox));

    auto* enclosureLog = new GeoLogVol("target_enclosure", enclosureShape, steel316L);
    return new GeoPhysVol(enclosureLog);
}

GeoPhysVol* TargetFactory::createHeVolume() {
    const GeoMaterial* pressurisedHe90 = m_materials.requireMaterial("PressurisedHe90");
    const GeoMaterial* tantalum = m_materials.requireMaterial("Tantalum");
    const GeoMaterial* tungsten = m_materials.requireMaterial("Tungsten");

    // Create HeVolume container
    auto* heVolumeTube = new GeoTube(0.0, s_heVolumeRadius, s_heVolumeHalfZ);
    auto* heVolumeLog = new GeoLogVol("HeVolume", heVolumeTube, pressurisedHe90);
    auto* heVolumePhys = new GeoPhysVol(heVolumeLog);

    // Create and place target enclosure
    auto* targetEnclosure = createTargetEnclosure();
    heVolumePhys->add(new GeoNameTag("target_enclosure"));
    heVolumePhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    heVolumePhys->add(targetEnclosure);

    // Create and place all 19 target slabs
    for (int i = 0; i < s_numSlabs; ++i) {
        // Create cladding tube (Tantalum)
        auto* claddingTube = new GeoTube(0.0, s_claddingRadius, s_claddingHalfZ[i]);
        std::string claddingName = "CladdedTarget_" + std::to_string(i + 1);
        auto* claddingLog = new GeoLogVol(claddingName, claddingTube, tantalum);
        auto* claddingPhys = new GeoPhysVol(claddingLog);

        // Create core tube (Tungsten)
        auto* coreTube = new GeoTube(0.0, s_coreRadius, s_coreHalfZ[i]);
        std::string coreName = "TargetCore_" + std::to_string(i + 1);
        auto* coreLog = new GeoLogVol(coreName, coreTube, tungsten);
        auto* corePhys = new GeoPhysVol(coreLog);

        // Place core inside cladding (centered)
        claddingPhys->add(new GeoNameTag(coreName));
        claddingPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
        claddingPhys->add(corePhys);

        // Place cladding in HeVolume
        GeoTrf::Transform3D slabTrf = GeoTrf::Translate3D(0.0, 0.0, s_slabPosZ[i]);
        heVolumePhys->add(new GeoNameTag(claddingName));
        heVolumePhys->add(new GeoTransform(slabTrf));
        heVolumePhys->add(claddingPhys);
    }

    return heVolumePhys;
}

}  // namespace SHiPGeometry
