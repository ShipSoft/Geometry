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

using units::gm;

TargetFactory::TargetFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TargetFactory::build() {
    // Get materials with null checking
    const GeoMaterial* vacuum = m_materials.requireMaterial("Vacuum");

    // Create target_vacuum_box (main container)
    auto* vacuumBox = new GeoBox(gm(s_vacuumBoxHalfX), gm(s_vacuumBoxHalfY), gm(s_vacuumBoxHalfZ));
    auto* vacuumBoxLog = new GeoLogVol("/SHiP/target", vacuumBox, vacuum);
    auto* vacuumBoxPhys = new GeoPhysVol(vacuumBoxLog);

    // Create and place proximity shielding
    auto* proximityShielding = createProximityShielding();
    GeoTrf::Transform3D proxTrf = GeoTrf::Translate3D(0.0, gm(s_proxPosY), 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/proximity_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(0));
    vacuumBoxPhys->add(new GeoTransform(proxTrf));
    vacuumBoxPhys->add(proximityShielding);

    // Create and place top shielding
    auto* topShielding = createTopShielding();
    GeoTrf::Transform3D topTrf = GeoTrf::Translate3D(0.0, gm(s_topShieldPosY), 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/top_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(1));
    vacuumBoxPhys->add(new GeoTransform(topTrf));
    vacuumBoxPhys->add(topShielding);

    // Create and place bottom shielding
    auto* bottomShielding = createBottomShielding();
    GeoTrf::Transform3D bottomTrf = GeoTrf::Translate3D(0.0, gm(s_bottomShieldPosY), 0.0);
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/bottom_shielding"));
    vacuumBoxPhys->add(new GeoIdentifierTag(2));
    vacuumBoxPhys->add(new GeoTransform(bottomTrf));
    vacuumBoxPhys->add(bottomShielding);

    // Create and place shielding pedestal
    auto* shieldingPedestal = createShieldingPedestal();
    GeoTrf::Transform3D pedestalTrf =
        GeoTrf::Translate3D(0.0, gm(s_pedestalPosY), gm(s_pedestalPosZ));
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
        GeoTrf::Translate3D(0.0, gm(s_targetAreaPosY), gm(s_targetAreaPosZ + s_heCentreZ));
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/he_volume"));
    vacuumBoxPhys->add(new GeoIdentifierTag(4));
    vacuumBoxPhys->add(new GeoTransform(heVolumeTrf));
    vacuumBoxPhys->add(heVolume);

    // Upstream cover plate remainder outside the He container: rectangular
    // plate, asymmetric about the beam axis, with a central hole covering
    // the He container radius; the stepped bore is modelled by the rings
    // inside HeVolume
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");
    auto* coverBox =
        new GeoBox(gm(s_coverPlateHalfX), gm(s_coverPlateHalfY), gm(s_coverPlateHalfZ));
    auto* coverHole = new GeoTube(0.0, gm(s_heRadius), gm(s_coverPlateHalfZ + 1.0 * mm));
    GeoTrf::Transform3D coverHoleTrf = GeoTrf::Translate3D(0.0, gm(-s_coverPlateOffsetY), 0.0);
    const GeoShape* coverShape = &(coverBox->subtract((*coverHole) << coverHoleTrf));
    auto* coverLog = new GeoLogVol("/SHiP/target/cover_plate", coverShape, steel316L);
    const auto coverCentreZ = 0.5 * (s_heZMin + s_coverZMax);
    GeoTrf::Transform3D coverTrf = GeoTrf::Translate3D(
        0.0, gm(s_targetAreaPosY + s_coverPlateOffsetY), gm(s_targetAreaPosZ + coverCentreZ));
    vacuumBoxPhys->add(new GeoNameTag("/SHiP/target/cover_plate"));
    vacuumBoxPhys->add(new GeoIdentifierTag(5));
    vacuumBoxPhys->add(new GeoTransform(coverTrf));
    vacuumBoxPhys->add(new GeoPhysVol(coverLog));

    return vacuumBoxPhys;
}

GeoPhysVol* TargetFactory::createProximityShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    // Envelope box
    auto* envelope = new GeoBox(gm(s_proxEnvHalfX), gm(s_proxEnvHalfY), gm(s_proxEnvHalfZ));
    // Inner cutout box
    auto* inner = new GeoBox(gm(s_proxInnerHalfX), gm(s_proxInnerHalfY), gm(s_proxInnerHalfZ));
    // Beam hole
    auto* hole = new GeoTube(0.0, gm(s_proxHoleRadius), gm(s_proxHoleHalfZ));

    // First subtraction: envelope - inner
    GeoTrf::Transform3D innerTrf = GeoTrf::Translate3D(0.0, 0.0, gm(s_proxInnerOffsetZ));
    const GeoShape* shape1 = &(envelope->subtract((*inner) << innerTrf));

    // Second subtraction: shape1 - hole
    GeoTrf::Transform3D holeTrf =
        GeoTrf::Translate3D(0.0, gm(s_proxHoleOffsetY), gm(s_proxHoleOffsetZ));
    const GeoShape* proxShape = &(shape1->subtract((*hole) << holeTrf));

    auto* proxLog = new GeoLogVol("/SHiP/target/proximity_shielding", proxShape, copper);
    return new GeoPhysVol(proxLog);
}

GeoPhysVol* TargetFactory::createTopShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* topBox = new GeoBox(gm(s_topShieldHalfX), gm(s_topShieldHalfY), gm(s_topShieldHalfZ));
    auto* topLog = new GeoLogVol("/SHiP/target/top_shielding", topBox, copper);
    return new GeoPhysVol(topLog);
}

GeoPhysVol* TargetFactory::createBottomShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto* bottomBox =
        new GeoBox(gm(s_bottomShieldHalfX), gm(s_bottomShieldHalfY), gm(s_bottomShieldHalfZ));
    auto* bottomLog = new GeoLogVol("/SHiP/target/bottom_shielding", bottomBox, copper);
    return new GeoPhysVol(bottomLog);
}

GeoPhysVol* TargetFactory::createShieldingPedestal() {
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto* pedestalBox = new GeoBox(gm(s_pedestalHalfX), gm(s_pedestalHalfY), gm(s_pedestalHalfZ));
    auto* pedestalLog = new GeoLogVol("/SHiP/target/shielding_pedestal", pedestalBox, iron);
    return new GeoPhysVol(pedestalLog);
}

const GeoShape* TargetFactory::createSteelCoreShape() {
    // The core is a polycone in the target frame (z = 0 at disk-1 front
    // face): bore r 125 -> 157 at the rear block, outer r 195 (front step,
    // inside the flange) -> 207 -> 190 (rear step, inside the rear flange).
    auto* corePcon = new GeoPcon(gm(0.0 * deg), gm(360.0 * deg));
    corePcon->addPlane(gm(s_coreZMin), gm(s_coreBoreR1), gm(s_coreFrontOuterR));
    corePcon->addPlane(gm(s_coreFrontZMax), gm(s_coreBoreR1), gm(s_coreFrontOuterR));
    corePcon->addPlane(gm(s_coreFrontZMax), gm(s_coreBoreR1), gm(s_coreOuterR));
    corePcon->addPlane(gm(s_coreBoreStepZ), gm(s_coreBoreR1), gm(s_coreOuterR));
    corePcon->addPlane(gm(s_coreBoreStepZ), gm(s_coreBoreR2), gm(s_coreOuterR));
    corePcon->addPlane(gm(s_coreRearZMin), gm(s_coreBoreR2), gm(s_coreOuterR));
    corePcon->addPlane(gm(s_coreRearZMin), gm(s_coreBoreR2), gm(s_coreRearOuterR));
    corePcon->addPlane(gm(s_coreZMax), gm(s_coreBoreR2), gm(s_coreRearOuterR));

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
        subtractGroove(gm(s_grooveRmin), gm(s_grooveRmax), gm(seg[0]), gm(seg[1]),
                       gm(90.0 * deg) - 0.5 * gm(s_groovePhiWidth), gm(s_groovePhiWidth));
    }
    for (const auto& seg : s_groovesBottom) {
        subtractGroove(gm(s_grooveRmin), gm(s_grooveRmax), gm(seg[0]), gm(seg[1]),
                       gm(270.0 * deg) - 0.5 * gm(s_groovePhiWidth), gm(s_groovePhiWidth));
    }
    subtractGroove(gm(s_rearGrooveRmin), gm(s_rearGrooveRmax), gm(s_rearGrooveZMin),
                   gm(s_rearGrooveZMax), gm(90.0 * deg) - 0.5 * gm(s_rearGroovePhiWidth),
                   gm(s_rearGroovePhiWidth));

    return coreShape;
}

GeoPhysVol* TargetFactory::createHeVolume() {
    const GeoMaterial* pressurisedHe90 = m_materials.requireMaterial("PressurisedHe90");
    const GeoMaterial* tungsten = m_materials.requireMaterial("Tungsten");
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");

    // Create HeVolume container
    auto* heVolumeTube = new GeoTube(0.0, gm(s_heRadius), 0.5 * gm(s_heZMax - s_heZMin));
    auto* heVolumeLog = new GeoLogVol("/SHiP/target/he_volume", heVolumeTube, pressurisedHe90);
    auto* heVolumePhys = new GeoPhysVol(heVolumeLog);

    // The HeVolume tube is centred on the target-frame He centre; this
    // translation converts a target-frame z centre into the local frame
    auto spanTrf = [](double z0, double z1) {
        return GeoTrf::Translate3D(0.0, 0.0, 0.5 * (z0 + z1) - gm(s_heCentreZ));
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
        const double z0 = gm(s_diskZ[i][0]);
        const double z1 = gm(s_diskZ[i][1]);
        const double radius = (i == s_numDisks - 1) ? gm(s_lastDiskRadius) : gm(s_diskRadius);
        auto* diskTube = new GeoTube(0.0, radius, 0.5 * (z1 - z0));
        std::string diskName = "/SHiP/target/core_" + std::to_string(i + 1);
        auto* diskLog = new GeoLogVol(diskName, diskTube, tungsten);
        place(new GeoPhysVol(diskLog), diskName, spanTrf(z0, z1));
    }

    // Steel core with the He grooves subtracted; the shape is defined in the
    // target frame, so shift it by the He centre only
    auto* coreLog = new GeoLogVol("/SHiP/target/core_steel", createSteelCoreShape(), steel316L);
    place(new GeoPhysVol(coreLog), "/SHiP/target/core_steel",
          GeoTrf::Translate3D(0.0, 0.0, -gm(s_heCentreZ)));

    // Jacket tube and flanges
    auto* jacketTube =
        new GeoTube(gm(s_jacketRmin), gm(s_jacketRmax), 0.5 * gm(s_jacketZMax - s_jacketZMin));
    auto* jacketLog = new GeoLogVol("/SHiP/target/jacket", jacketTube, steel316L);
    place(new GeoPhysVol(jacketLog), "/SHiP/target/jacket",
          spanTrf(gm(s_jacketZMin), gm(s_jacketZMax)));

    auto* flangeFrontTube = new GeoTube(gm(s_flangeFrontRmin), gm(s_jacketRmax),
                                        0.5 * gm(s_jacketZMin - s_flangeFrontZMin));
    auto* flangeFrontLog = new GeoLogVol("/SHiP/target/flange_front", flangeFrontTube, steel316L);
    place(new GeoPhysVol(flangeFrontLog), "/SHiP/target/flange_front",
          spanTrf(gm(s_flangeFrontZMin), gm(s_jacketZMin)));

    // Beam window and its nose ring closing the vessel upstream
    auto* windowTube = new GeoTube(0.0, gm(s_windowRmax), 0.5 * gm(s_windowZMax - s_windowZMin));
    auto* windowLog = new GeoLogVol("/SHiP/target/front_window", windowTube, steel316L);
    place(new GeoPhysVol(windowLog), "/SHiP/target/front_window",
          spanTrf(gm(s_windowZMin), gm(s_windowZMax)));

    auto* noseTube = new GeoTube(gm(s_windowRmax), gm(s_flangeFrontRmin),
                                 0.5 * gm(s_flangeFrontZMin - s_noseZMin));
    auto* noseLog = new GeoLogVol("/SHiP/target/front_nose", noseTube, steel316L);
    place(new GeoPhysVol(noseLog), "/SHiP/target/front_nose",
          spanTrf(gm(s_noseZMin), gm(s_flangeFrontZMin)));

    // Cover plate bore rings (the part of the plate within the He container)
    auto* coverRing1Tube =
        new GeoTube(gm(s_coverRing1Rmin), gm(s_heRadius), 0.5 * gm(s_flangeFrontZMin - s_heZMin));
    auto* coverRing1Log = new GeoLogVol("/SHiP/target/cover_ring1", coverRing1Tube, steel316L);
    place(new GeoPhysVol(coverRing1Log), "/SHiP/target/cover_ring1",
          spanTrf(gm(s_heZMin), gm(s_flangeFrontZMin)));

    auto* coverRing2Tube = new GeoTube(gm(s_coverRing2Rmin), gm(s_heRadius),
                                       0.5 * gm(s_coverZMax - s_flangeFrontZMin));
    auto* coverRing2Log = new GeoLogVol("/SHiP/target/cover_ring2", coverRing2Tube, steel316L);
    place(new GeoPhysVol(coverRing2Log), "/SHiP/target/cover_ring2",
          spanTrf(gm(s_flangeFrontZMin), gm(s_coverZMax)));

    auto* flangeRearTube = new GeoTube(gm(s_jacketRmax), gm(s_flangeRearRmax),
                                       0.5 * gm(s_flangeRearZMax - s_jacketZMax));
    auto* flangeRearLog = new GeoLogVol("/SHiP/target/flange_back", flangeRearTube, steel316L);
    place(new GeoPhysVol(flangeRearLog), "/SHiP/target/flange_back",
          spanTrf(gm(s_jacketZMax), gm(s_flangeRearZMax)));

    // Rear endcap as a single polycone in the target frame
    auto* endcapPcon = new GeoPcon(gm(0.0 * deg), gm(360.0 * deg));
    for (const auto& plane : s_endcapPlanes) {
        endcapPcon->addPlane(gm(plane[0]), gm(plane[1]), gm(plane[2]));
    }
    auto* endcapLog = new GeoLogVol("/SHiP/target/endcap", endcapPcon, steel316L);
    place(new GeoPhysVol(endcapLog), "/SHiP/target/endcap",
          GeoTrf::Translate3D(0.0, 0.0, -gm(s_heCentreZ)));

    return heVolumePhys;
}

}  // namespace SHiPGeometry
