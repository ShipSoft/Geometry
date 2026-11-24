// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Target/TargetFactory.h"

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/Placement.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoPcon.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoTubs.h>

#include <string>

namespace SHiP::geometry {

using units::gm;

TargetFactory::TargetFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* TargetFactory::build() {
    // Get materials with null checking
    const GeoMaterial* vacuum = m_materials.requireMaterial("Vacuum");

    // Create target_vacuum_box (main container)
    auto const* vacuumBox =
        new GeoBox(gm(s_vacuumBoxHalfX), gm(s_vacuumBoxHalfY), gm(s_vacuumBoxHalfZ));
    auto* vacuumBoxPhys = makeVolume("/SHiP/target", vacuumBox, vacuum);
    // Children are numbered in placement order.
    Placer placer{vacuumBoxPhys};

    // Create and place proximity shielding
    auto* proximityShielding = createProximityShielding();
    GeoTrf::Transform3D const proxTrf = GeoTrf::Translate3D(0.0, gm(s_proxPosY), 0.0);
    placer.place(proximityShielding, proxTrf);

    // Create and place top shielding
    auto* topShielding = createTopShielding();
    GeoTrf::Transform3D const topTrf = GeoTrf::Translate3D(0.0, gm(s_topShieldPosY), 0.0);
    placer.place(topShielding, topTrf);

    // Create and place bottom shielding
    auto* bottomShielding = createBottomShielding();
    GeoTrf::Transform3D const bottomTrf = GeoTrf::Translate3D(0.0, gm(s_bottomShieldPosY), 0.0);
    placer.place(bottomShielding, bottomTrf);

    // Create and place shielding pedestal
    auto* shieldingPedestal = createShieldingPedestal();
    GeoTrf::Transform3D const pedestalTrf =
        GeoTrf::Translate3D(0.0, gm(s_pedestalPosY), gm(s_pedestalPosZ));
    placer.place(shieldingPedestal, pedestalTrf);

    // Create and place HeVolume (contains disks, steel core, jacket, flanges
    // and rear endcap). The tube is centred on the target-frame He centre,
    // and the target frame sits at (s_targetAreaPosY, s_targetAreaPosZ) in
    // the vacuum box.
    auto* heVolume = createHeVolume();
    const GeoTrf::Transform3D heVolumeTrf =
        GeoTrf::Translate3D(0.0, gm(s_targetAreaPosY), gm(s_targetAreaPosZ + s_heCentreZ));
    placer.place(heVolume, heVolumeTrf);

    // Upstream cover plate remainder outside the He container: rectangular
    // plate, asymmetric about the beam axis, with a central hole covering
    // the He container radius; the stepped bore is modelled by the rings
    // inside HeVolume
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");
    auto const* coverBox =
        new GeoBox(gm(s_coverPlateHalfX), gm(s_coverPlateHalfY), gm(s_coverPlateHalfZ));
    auto const* coverHole = new GeoTube(0.0, gm(s_heRadius), gm(s_coverPlateHalfZ + 1.0 * mm));
    GeoTrf::Transform3D const coverHoleTrf =
        GeoTrf::Translate3D(0.0, gm(-s_coverPlateOffsetY), 0.0);
    const GeoShape* coverShape = &coverBox->subtract((*coverHole) << coverHoleTrf);
    const auto coverCentreZ = 0.5 * (s_heZMin + s_coverZMax);
    GeoTrf::Transform3D const coverTrf = GeoTrf::Translate3D(
        0.0, gm(s_targetAreaPosY + s_coverPlateOffsetY), gm(s_targetAreaPosZ + coverCentreZ));
    placer.place(makeVolume("/SHiP/target/cover_plate", coverShape, steel316L), coverTrf);

    return vacuumBoxPhys;
}

GeoPhysVol* TargetFactory::createProximityShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    // Envelope box
    auto const* envelope = new GeoBox(gm(s_proxEnvHalfX), gm(s_proxEnvHalfY), gm(s_proxEnvHalfZ));
    // Inner cutout box
    auto const* inner =
        new GeoBox(gm(s_proxInnerHalfX), gm(s_proxInnerHalfY), gm(s_proxInnerHalfZ));
    // Beam hole
    auto const* hole = new GeoTube(0.0, gm(s_proxHoleRadius), gm(s_proxHoleHalfZ));

    // First subtraction: envelope - inner
    GeoTrf::Transform3D const innerTrf = GeoTrf::Translate3D(0.0, 0.0, gm(s_proxInnerOffsetZ));
    const GeoShape* shape1 = &envelope->subtract((*inner) << innerTrf);

    // Second subtraction: shape1 - hole
    GeoTrf::Transform3D const holeTrf =
        GeoTrf::Translate3D(0.0, gm(s_proxHoleOffsetY), gm(s_proxHoleOffsetZ));
    const GeoShape* proxShape = &shape1->subtract((*hole) << holeTrf);

    return makeVolume("/SHiP/target/proximity_shielding", proxShape, copper);
}

GeoPhysVol* TargetFactory::createTopShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto const* topBox =
        new GeoBox(gm(s_topShieldHalfX), gm(s_topShieldHalfY), gm(s_topShieldHalfZ));
    return makeVolume("/SHiP/target/top_shielding", topBox, copper);
}

GeoPhysVol* TargetFactory::createBottomShielding() {
    const GeoMaterial* copper = m_materials.requireMaterial("Copper");

    auto const* bottomBox =
        new GeoBox(gm(s_bottomShieldHalfX), gm(s_bottomShieldHalfY), gm(s_bottomShieldHalfZ));
    return makeVolume("/SHiP/target/bottom_shielding", bottomBox, copper);
}

GeoPhysVol* TargetFactory::createShieldingPedestal() {
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto const* pedestalBox =
        new GeoBox(gm(s_pedestalHalfX), gm(s_pedestalHalfY), gm(s_pedestalHalfZ));
    return makeVolume("/SHiP/target/shielding_pedestal", pedestalBox, iron);
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
    const auto subtractGroove = [&coreShape](double rmin, double rmax, double z0, double z1,
                                             double phiStart, double phiWidth) {
        const auto* tubs = new GeoTubs(rmin, rmax, 0.5 * (z1 - z0), phiStart, phiWidth);
        coreShape = &coreShape->subtract((*tubs) << GeoTrf::Translate3D(0.0, 0.0, 0.5 * (z0 + z1)));
    };
    for (const auto& seg : s_groovesTop) {
        subtractGroove(gm(s_grooveRmin), gm(s_grooveRmax), gm(seg[0]), gm(seg[1]),
                       gm(90.0 * deg) - (0.5 * gm(s_groovePhiWidth)), gm(s_groovePhiWidth));
    }
    for (const auto& seg : s_groovesBottom) {
        subtractGroove(gm(s_grooveRmin), gm(s_grooveRmax), gm(seg[0]), gm(seg[1]),
                       gm(270.0 * deg) - (0.5 * gm(s_groovePhiWidth)), gm(s_groovePhiWidth));
    }
    subtractGroove(gm(s_rearGrooveRmin), gm(s_rearGrooveRmax), gm(s_rearGrooveZMin),
                   gm(s_rearGrooveZMax), gm(90.0 * deg) - (0.5 * gm(s_rearGroovePhiWidth)),
                   gm(s_rearGroovePhiWidth));

    return coreShape;
}

GeoPhysVol* TargetFactory::createHeVolume() {
    const GeoMaterial* pressurisedHe90 = m_materials.requireMaterial("PressurisedHe90");
    const GeoMaterial* tungsten = m_materials.requireMaterial("Tungsten");
    const GeoMaterial* steel316L = m_materials.requireMaterial("Steel316L");

    // Create HeVolume container
    auto const* heVolumeTube = new GeoTube(0.0, gm(s_heRadius), 0.5 * gm(s_heZMax - s_heZMin));
    auto* heVolumePhys = makeVolume("/SHiP/target/he_volume", heVolumeTube, pressurisedHe90);

    // The HeVolume tube is centred on the target-frame He centre; this
    // translation converts a target-frame z centre into the local frame
    const auto spanTrf = [](double z0, double z1) {
        return GeoTrf::Translate3D(0.0, 0.0, (0.5 * (z0 + z1)) - gm(s_heCentreZ));
    };
    // Children are numbered in placement order.
    Placer placer{heVolumePhys};

    // Tungsten disks (no cladding); the last disk (rear block) is larger
    for (int i = 0; i < s_numDisks; ++i) {
        const double z0 = gm(s_diskZ[i][0]);
        const double z1 = gm(s_diskZ[i][1]);
        const double radius = (i == s_numDisks - 1) ? gm(s_lastDiskRadius) : gm(s_diskRadius);
        const auto* diskTube = new GeoTube(0.0, radius, 0.5 * (z1 - z0));
        const std::string diskName = "/SHiP/target/core_" + std::to_string(i + 1);
        placer.place(makeVolume(diskName, diskTube, tungsten), spanTrf(z0, z1));
    }

    // Steel core with the He grooves subtracted; the shape is defined in the
    // target frame, so shift it by the He centre only
    placer.place(makeVolume("/SHiP/target/core_steel", createSteelCoreShape(), steel316L),
                 GeoTrf::Translate3D(0.0, 0.0, -gm(s_heCentreZ)));

    // Jacket tube and flanges
    auto const* jacketTube =
        new GeoTube(gm(s_jacketRmin), gm(s_jacketRmax), 0.5 * gm(s_jacketZMax - s_jacketZMin));
    placer.place(makeVolume("/SHiP/target/jacket", jacketTube, steel316L),
                 spanTrf(gm(s_jacketZMin), gm(s_jacketZMax)));

    auto const* flangeFrontTube = new GeoTube(gm(s_flangeFrontRmin), gm(s_jacketRmax),
                                              0.5 * gm(s_jacketZMin - s_flangeFrontZMin));
    placer.place(makeVolume("/SHiP/target/flange_front", flangeFrontTube, steel316L),
                 spanTrf(gm(s_flangeFrontZMin), gm(s_jacketZMin)));

    // Beam window and its nose ring closing the vessel upstream
    auto const* windowTube =
        new GeoTube(0.0, gm(s_windowRmax), 0.5 * gm(s_windowZMax - s_windowZMin));
    placer.place(makeVolume("/SHiP/target/front_window", windowTube, steel316L),
                 spanTrf(gm(s_windowZMin), gm(s_windowZMax)));

    auto const* noseTube = new GeoTube(gm(s_windowRmax), gm(s_flangeFrontRmin),
                                       0.5 * gm(s_flangeFrontZMin - s_noseZMin));
    placer.place(makeVolume("/SHiP/target/front_nose", noseTube, steel316L),
                 spanTrf(gm(s_noseZMin), gm(s_flangeFrontZMin)));

    // Cover plate bore rings (the part of the plate within the He container)
    const auto* coverRing1Tube =
        new GeoTube(gm(s_coverRing1Rmin), gm(s_heRadius), 0.5 * gm(s_flangeFrontZMin - s_heZMin));
    placer.place(makeVolume("/SHiP/target/cover_ring1", coverRing1Tube, steel316L),
                 spanTrf(gm(s_heZMin), gm(s_flangeFrontZMin)));

    auto const* coverRing2Tube = new GeoTube(gm(s_coverRing2Rmin), gm(s_heRadius),
                                             0.5 * gm(s_coverZMax - s_flangeFrontZMin));
    placer.place(makeVolume("/SHiP/target/cover_ring2", coverRing2Tube, steel316L),
                 spanTrf(gm(s_flangeFrontZMin), gm(s_coverZMax)));

    auto const* flangeRearTube = new GeoTube(gm(s_jacketRmax), gm(s_flangeRearRmax),
                                             0.5 * gm(s_flangeRearZMax - s_jacketZMax));
    placer.place(makeVolume("/SHiP/target/flange_back", flangeRearTube, steel316L),
                 spanTrf(gm(s_jacketZMax), gm(s_flangeRearZMax)));

    // Rear endcap as a single polycone in the target frame
    auto* endcapPcon = new GeoPcon(gm(0.0 * deg), gm(360.0 * deg));
    for (const auto& plane : s_endcapPlanes) {
        endcapPcon->addPlane(gm(plane[0]), gm(plane[1]), gm(plane[2]));
    }
    placer.place(makeVolume("/SHiP/target/endcap", endcapPcon, steel316L),
                 GeoTrf::Translate3D(0.0, 0.0, -gm(s_heCentreZ)));

    return heVolumePhys;
}

REGISTER_SUBSYSTEM(TargetFactory)

}  // namespace SHiP::geometry
