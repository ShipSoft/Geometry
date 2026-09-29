// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Target/TargetFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPcon.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoTubs.h>

#include <string>

namespace SHiPGeometry {

TargetFactory::TargetFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TargetFactory::build() {
    // Get materials with null checking
    const GeoMaterial* vacuum = m_materials.requireMaterial("Vacuum");

    // Create target_vacuum_box (main container)
    auto* vacuumBox = new GeoBox(s_vacuumBoxHalfX, s_vacuumBoxHalfY, s_vacuumBoxHalfZ);
    auto* vacuumBoxLog = new GeoLogVol("/SHiP/target", vacuumBox, vacuum);
    auto* vacuumBoxPhys = new GeoPhysVol(vacuumBoxLog);

    // Create and place proximity shielding
    auto* proximityShielding = createProximityShielding();
    GeoTrf::Transform3D proxTrf = GeoTrf::Translate3D(0.0, s_proxPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/proximity_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(0));
    vacuumBoxPhys->add(new GeoTransform(proxTrf));
    vacuumBoxPhys->add(proximityShielding);

    // Create and place top shielding
    auto* topShielding = createTopShielding();
    GeoTrf::Transform3D topTrf = GeoTrf::Translate3D(0.0, s_topShieldPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/top_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(1));
    vacuumBoxPhys->add(new GeoTransform(topTrf));
    vacuumBoxPhys->add(topShielding);

    // Create and place bottom shielding
    auto* bottomShielding = createBottomShielding();
    GeoTrf::Transform3D bottomTrf = GeoTrf::Translate3D(0.0, s_bottomShieldPosY, 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/bottom_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(2));
    vacuumBoxPhys->add(new GeoTransform(bottomTrf));
    vacuumBoxPhys->add(bottomShielding);

    // Create and place shielding pedestal
    auto* shieldingPedestal = createShieldingPedestal();
    GeoTrf::Transform3D pedestalTrf = GeoTrf::Translate3D(0.0, s_pedestalPosY, s_pedestalPosZ);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/shielding_pedestal"));
    vacuumBoxPhys->add(new GeoIdentifierTag(3));
    vacuumBoxPhys->add(new GeoTransform(pedestalTrf));
    vacuumBoxPhys->add(shieldingPedestal);

    // Create and place HeVolume (contains disks, steel core, jacket, flanges
    // and rear endcap). The tube is centred on the target-frame He centre,
    // and the target frame sits at (s_targetAreaPosY, s_targetAreaPosZ) in
    // the vacuum box.
    auto* heVolume = createHeVolume();
    GeoTrf::Transform3D heVolumeTrf =
        GeoTrf::Translate3D(0.0, s_targetAreaPosY, s_targetAreaPosZ + s_heCentreZ);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/he_volume"));
    vacuumBoxPhys->add(new GeoIdentifierTag(4));
    vacuumBoxPhys->add(new GeoTransform(heVolumeTrf));
    vacuumBoxPhys->add(heVolume);

    // Upstream cover plate remainder outside the He container: rectangular
    // plate, asymmetric about the beam axis, with a central hole covering
    // the He container radius; the stepped bore is modelled by the rings
    // inside HeVolume
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");
    auto* coverBox = new GeoBox(s_coverPlateHalfX, s_coverPlateHalfY, s_coverPlateHalfZ);
    auto* coverHole = new GeoTube(0.0, s_heRadius, s_coverPlateHalfZ + 1.0 * mm);
    GeoTrf::Transform3D coverHoleTrf = GeoTrf::Translate3D(0.0, -s_coverPlateOffsetY, 0.0);
    const GeoShape* coverShape = &(coverBox->subtract((*coverHole) << coverHoleTrf));
    auto* coverLog = new GeoLogVol("/SHiP/target/cover_plate", coverShape, steel316L);
    const double coverCentreZ = 0.5 * (s_heZMin + s_coverZMax);
    GeoTrf::Transform3D coverTrf = GeoTrf::Translate3D(0.0, s_targetAreaPosY + s_coverPlateOffsetY,
                                                       s_targetAreaPosZ + coverCentreZ);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/cover_plate"));
    vacuumBoxPhys->add(new GeoIdentifierTag(5));
    vacuumBoxPhys->add(new GeoTransform(coverTrf));
    vacuumBoxPhys->add(new GeoPhysVol(coverLog));

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

    auto* proxLog = new GeoLogVol("/SHiP/target/proximity_shielding", proxShape, copper);
    return new GeoPhysVol(proxLog);
}

GeoPhysVol* TargetFactory::createTopShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* topBox = new GeoBox(s_topShieldHalfX, s_topShieldHalfY, s_topShieldHalfZ);
    auto* topLog = new GeoLogVol("/SHiP/target/top_shielding", topBox, copper);
    return new GeoPhysVol(topLog);
}

GeoPhysVol* TargetFactory::createBottomShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* bottomBox = new GeoBox(s_bottomShieldHalfX, s_bottomShieldHalfY, s_bottomShieldHalfZ);
    auto* bottomLog = new GeoLogVol("/SHiP/target/bottom_shielding", bottomBox, copper);
    return new GeoPhysVol(bottomLog);
}

GeoPhysVol* TargetFactory::createShieldingPedestal() {
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto* pedestalBox = new GeoBox(s_pedestalHalfX, s_pedestalHalfY, s_pedestalHalfZ);
    auto* pedestalLog = new GeoLogVol("/SHiP/target/shielding_pedestal", pedestalBox, iron);
    return new GeoPhysVol(pedestalLog);
}

const GeoShape* TargetFactory::createSteelCoreShape() {
    // The core is a polycone in the target frame (z = 0 at disk-1 front
    // face): bore r 125 -> 157 at the rear block, outer r 195 (front step,
    // inside the flange) -> 207 -> 190 (rear step, inside the rear flange).
    auto* corePcon = new GeoPcon(0.0 * deg, 360.0 * deg);
    corePcon->addPlane(s_coreZMin, s_coreBoreR1, s_coreFrontOuterR);
    corePcon->addPlane(s_coreFrontZMax, s_coreBoreR1, s_coreFrontOuterR);
    corePcon->addPlane(s_coreFrontZMax, s_coreBoreR1, s_coreOuterR);
    corePcon->addPlane(s_coreBoreStepZ, s_coreBoreR1, s_coreOuterR);
    corePcon->addPlane(s_coreBoreStepZ, s_coreBoreR2, s_coreOuterR);
    corePcon->addPlane(s_coreRearZMin, s_coreBoreR2, s_coreOuterR);
    corePcon->addPlane(s_coreRearZMin, s_coreBoreR2, s_coreRearOuterR);
    corePcon->addPlane(s_coreZMax, s_coreBoreR2, s_coreRearOuterR);

    // Subtract the He cooling grooves (tube segments centred on the vertical
    // axis; the removed volume fills with the parent helium)
    const GeoShape* coreShape = corePcon;
    auto subtractGroove = [&coreShape](double rmin, double rmax, double z0, double z1,
                                       double phiStart, double phiWidth) {
        auto* tubs = new GeoTubs(rmin, rmax, 0.5 * (z1 - z0), phiStart, phiWidth);
        coreShape =
            &(coreShape->subtract((*tubs) << GeoTrf::Translate3D(0.0, 0.0, 0.5 * (z0 + z1))));
    };
    for (const auto& seg : s_groovesTop) {
        subtractGroove(s_grooveRmin, s_grooveRmax, seg[0], seg[1],
                       90.0 * deg - 0.5 * s_groovePhiWidth, s_groovePhiWidth);
    }
    for (const auto& seg : s_groovesBottom) {
        subtractGroove(s_grooveRmin, s_grooveRmax, seg[0], seg[1],
                       270.0 * deg - 0.5 * s_groovePhiWidth, s_groovePhiWidth);
    }
    subtractGroove(s_rearGrooveRmin, s_rearGrooveRmax, s_rearGrooveZMin, s_rearGrooveZMax,
                   90.0 * deg - 0.5 * s_rearGroovePhiWidth, s_rearGroovePhiWidth);

    return coreShape;
}

GeoPhysVol* TargetFactory::createHeVolume() {
    const GeoMaterial* pressurisedHe90 = m_materials.requireMaterial("PressurisedHe90");
    const GeoMaterial* tungsten = m_materials.requireMaterial("Tungsten");
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");

    // Create HeVolume container
    auto* heVolumeTube = new GeoTube(0.0, s_heRadius, 0.5 * (s_heZMax - s_heZMin));
    auto* heVolumeLog = new GeoLogVol("/SHiP/target/he_volume", heVolumeTube, pressurisedHe90);
    auto* heVolumePhys = new GeoPhysVol(heVolumeLog);

    // The HeVolume tube is centred on the target-frame He centre; this
    // translation converts a target-frame z centre into the local frame
    auto spanTrf = [](double z0, double z1) {
        return GeoTrf::Translate3D(0.0, 0.0, 0.5 * (z0 + z1) - s_heCentreZ);
    };
    int nextId = 0;
    auto place = [&](GeoPhysVol* child, const std::string& name, GeoTrf::Transform3D trf) {
        heVolumePhys->add(new GeoNameTag(name));
        heVolumePhys->add(new GeoIdentifierTag(nextId++));
        heVolumePhys->add(new GeoTransform(trf));
        heVolumePhys->add(child);
    };

    // Tungsten disks (no cladding); the last disk (rear block) is larger
    for (int i = 0; i < s_numDisks; ++i) {
        const double z0 = s_diskZ[i][0];
        const double z1 = s_diskZ[i][1];
        const double radius = (i == s_numDisks - 1) ? s_lastDiskRadius : s_diskRadius;
        auto* diskTube = new GeoTube(0.0, radius, 0.5 * (z1 - z0));
        std::string diskName = "/SHiP/target/core_" + std::to_string(i + 1);
        auto* diskLog = new GeoLogVol(diskName, diskTube, tungsten);
        place(new GeoPhysVol(diskLog), diskName, spanTrf(z0, z1));
    }

    // Steel core with the He grooves subtracted; the shape is defined in the
    // target frame, so shift it by the He centre only
    auto* coreLog = new GeoLogVol("/SHiP/target/core_steel", createSteelCoreShape(), steel316L);
    place(new GeoPhysVol(coreLog), "/SHiP/target/core_steel",
          GeoTrf::Translate3D(0.0, 0.0, -s_heCentreZ));

    // Jacket tube and flanges
    auto* jacketTube = new GeoTube(s_jacketRmin, s_jacketRmax, 0.5 * (s_jacketZMax - s_jacketZMin));
    auto* jacketLog = new GeoLogVol("/SHiP/target/jacket", jacketTube, steel316L);
    place(new GeoPhysVol(jacketLog), "/SHiP/target/jacket", spanTrf(s_jacketZMin, s_jacketZMax));

    auto* flangeFrontTube =
        new GeoTube(s_flangeFrontRmin, s_jacketRmax, 0.5 * (s_jacketZMin - s_flangeFrontZMin));
    auto* flangeFrontLog = new GeoLogVol("/SHiP/target/flange_front", flangeFrontTube, steel316L);
    place(new GeoPhysVol(flangeFrontLog), "/SHiP/target/flange_front",
          spanTrf(s_flangeFrontZMin, s_jacketZMin));

    // Beam window and its nose ring closing the vessel upstream
    auto* windowTube = new GeoTube(0.0, s_windowRmax, 0.5 * (s_windowZMax - s_windowZMin));
    auto* windowLog = new GeoLogVol("/SHiP/target/front_window", windowTube, steel316L);
    place(new GeoPhysVol(windowLog), "/SHiP/target/front_window",
          spanTrf(s_windowZMin, s_windowZMax));

    auto* noseTube =
        new GeoTube(s_windowRmax, s_flangeFrontRmin, 0.5 * (s_flangeFrontZMin - s_noseZMin));
    auto* noseLog = new GeoLogVol("/SHiP/target/front_nose", noseTube, steel316L);
    place(new GeoPhysVol(noseLog), "/SHiP/target/front_nose",
          spanTrf(s_noseZMin, s_flangeFrontZMin));

    // Cover plate bore rings (the part of the plate within the He container)
    auto* coverRing1Tube =
        new GeoTube(s_coverRing1Rmin, s_heRadius, 0.5 * (s_flangeFrontZMin - s_heZMin));
    auto* coverRing1Log = new GeoLogVol("/SHiP/target/cover_ring1", coverRing1Tube, steel316L);
    place(new GeoPhysVol(coverRing1Log), "/SHiP/target/cover_ring1",
          spanTrf(s_heZMin, s_flangeFrontZMin));

    auto* coverRing2Tube =
        new GeoTube(s_coverRing2Rmin, s_heRadius, 0.5 * (s_coverZMax - s_flangeFrontZMin));
    auto* coverRing2Log = new GeoLogVol("/SHiP/target/cover_ring2", coverRing2Tube, steel316L);
    place(new GeoPhysVol(coverRing2Log), "/SHiP/target/cover_ring2",
          spanTrf(s_flangeFrontZMin, s_coverZMax));

    auto* flangeRearTube =
        new GeoTube(s_jacketRmax, s_flangeRearRmax, 0.5 * (s_flangeRearZMax - s_jacketZMax));
    auto* flangeRearLog = new GeoLogVol("/SHiP/target/flange_back", flangeRearTube, steel316L);
    place(new GeoPhysVol(flangeRearLog), "/SHiP/target/flange_back",
          spanTrf(s_jacketZMax, s_flangeRearZMax));

    // Rear endcap as a single polycone in the target frame
    auto* endcapPcon = new GeoPcon(0.0 * deg, 360.0 * deg);
    for (const auto& plane : s_endcapPlanes) {
        endcapPcon->addPlane(plane[0], plane[1], plane[2]);
    }
    auto* endcapLog = new GeoLogVol("/SHiP/target/endcap", endcapPcon, steel316L);
    place(new GeoPhysVol(endcapLog), "/SHiP/target/endcap",
          GeoTrf::Translate3D(0.0, 0.0, -s_heCentreZ));

    return heVolumePhys;
}

}  // namespace SHiPGeometry
