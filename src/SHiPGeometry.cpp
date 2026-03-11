// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include "Calorimeter/CalorimeterFactory.h"
#include "Cavern/CavernFactory.h"
#include "DecayVolume/DecayVolumeFactory.h"
#include "Magnet/MagnetFactory.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"
#include "Target/TargetFactory.h"
#include "TimingDetector/TimingDetectorFactory.h"
#include "Trackers/TrackersFactory.h"
#include "UpstreamTagger/SHiPUBTManager.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

namespace SHiPGeometry {

SHiPGeometryBuilder::SHiPGeometryBuilder() = default;
SHiPGeometryBuilder::~SHiPGeometryBuilder() = default;

GeoPhysVol* SHiPGeometryBuilder::build() {
    // Create central materials manager
    SHiPMaterials materials;

    // Build the cavern (world volume)
    CavernFactory cavernFactory(materials);
    GeoPhysVol* world = cavernFactory.build();

    // Build and place the Target
    TargetFactory targetFactory(materials);
    GeoPhysVol* target = targetFactory.build();

    // Position target in world (from GDML: x=0, y=-14.45cm, z=43.25cm)
    // Note: These are relative to the cave origin
    constexpr double cm = GeoModelKernelUnits::cm;
    GeoTrf::Transform3D targetTrf = GeoTrf::Translate3D(0.0, -14.45 * cm, 43.25 * cm);
    world->add(new GeoNameTag("target_vacuum_box"));
    world->add(new GeoTransform(targetTrf));
    world->add(target);

    // Build and place MuonShieldArea
    // GDML z range: 204–3148.66 cm → centre: 1676.33 cm = 16763.3 mm from world origin
    MuonShieldFactory muonShieldFactory(materials);
    GeoPhysVol* muonShield = muonShieldFactory.build();
    GeoTrf::Transform3D muonShieldTrf = GeoTrf::Translate3D(0.0, 0.0, 16763.3);
    world->add(new GeoNameTag("MuonShieldArea"));
    world->add(new GeoTransform(muonShieldTrf));
    world->add(muonShield);

    // Build and place UpstreamTagger (sensitive scintillator slab)
    // Z: 32.52 to 32.92 m → centre: 32.72 m
    SHiPUBTManager ubtManager;
    UpstreamTaggerFactory upstreamTaggerFactory(materials);
    GeoVPhysVol* upstreamTagger = upstreamTaggerFactory.build(&ubtManager);
    GeoTrf::Transform3D upstreamTaggerTrf = GeoTrf::Translate3D(0.0, 0.0, 32.72 * 1000.0);
    world->add(new GeoNameTag("Upstream_Tagger"));
    world->add(new GeoTransform(upstreamTaggerTrf));
    world->add(upstreamTagger);

    // Build and place DecayVolume
    // Z: 32.92 to 83.32 m → centre: 58.12 m
    DecayVolumeFactory decayVolumeFactory(materials);
    GeoPhysVol* decayVolume = decayVolumeFactory.build();
    GeoTrf::Transform3D decayVolumeTrf =
        GeoTrf::Translate3D(0.0, 0.0, 58.12 * 1000.0);  // Convert m to mm
    world->add(new GeoNameTag("DecayVolume"));
    world->add(new GeoTransform(decayVolumeTrf));
    world->add(decayVolume);

    // Build and place Trackers (container with 4 stations)
    // Container spans stations 1-4, centred appropriately
    TrackersFactory trackersFactory(materials);
    GeoPhysVol* trackers = trackersFactory.build();
    // The TrackerFactory already handles internal positioning, just place the container
    // at its calculated centre Z position
    constexpr double trackersCentreZ =
        (84.07 + 95.07) / 2.0 * 1000.0;  // Average of station 1 and 4 centres
    GeoTrf::Transform3D trackersTrf = GeoTrf::Translate3D(0.0, 0.0, trackersCentreZ);
    world->add(new GeoNameTag("TrackersContainer"));
    world->add(new GeoTransform(trackersTrf));
    world->add(trackers);

    // Build and place Magnet
    // Z: 87.07 to 92.07 m → centre: 89.57 m
    MagnetFactory magnetFactory(materials);
    GeoPhysVol* magnet = magnetFactory.build();
    GeoTrf::Transform3D magnetTrf =
        GeoTrf::Translate3D(0.0, 0.0, 89.57 * 1000.0);  // Convert m to mm
    world->add(new GeoNameTag("Magnet"));
    world->add(new GeoTransform(magnetTrf));
    world->add(magnet);

    // Build and place TimingDetector
    // Z: 95.902 m (from GDML reference)
    TimingDetectorFactory timingDetectorFactory(materials);
    GeoPhysVol* timingDetector = timingDetectorFactory.build();
    GeoTrf::Transform3D timingDetectorTrf =
        GeoTrf::Translate3D(0.0, 0.0, 95.902 * 1000.0);  // Convert m to mm
    world->add(new GeoNameTag("TimingDetector"));
    world->add(new GeoTransform(timingDetectorTrf));
    world->add(timingDetector);

    // Build and place Calorimeter (container with ECAL front, ECAL back, HCAL)
    // Full calorimeter spans Z: 96.87 to 99.77 m → centre: 98.32 m
    CalorimeterFactory calorimeterFactory(materials);
    GeoPhysVol* calorimeter = calorimeterFactory.build();
    GeoTrf::Transform3D calorimeterTrf =
        GeoTrf::Translate3D(0.0, 0.0, 98.32 * 1000.0);  // Convert m to mm
    world->add(new GeoNameTag("CalorimeterContainer"));
    world->add(new GeoTransform(calorimeterTrf));
    world->add(calorimeter);

    return world;
}

}  // namespace SHiPGeometry
