// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include "Calorimeter/CalorimeterFactory.h"
#include "Cavern/CavernFactory.h"
#include "DecayVolume/DecayVolumeFactory.h"
#include "Magnet/MagnetFactory.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/Placement.h"
#include "SHiPGeometry/SHiPMaterials.h"
#include "Target/TargetFactory.h"
#include "TimingDetector/TimingDetectorFactory.h"
#include "Trackers/TrackersFactory.h"
#include "UpstreamTagger/SHiPUBTManager.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoPhysVol.h>
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
    placeChild(world, target, "/SHiP/target", 1, GeoTrf::Translate3D(0.0, -14.45 * cm, 43.25 * cm));

    // Build and place MuonShieldArea
    // GDML z range: 204–3148.66 cm → centre: 1676.33 cm = 16763.3 mm from world origin
    MuonShieldFactory muonShieldFactory(materials);
    GeoPhysVol* muonShield = muonShieldFactory.build();
    placeChild(world, muonShield, "/SHiP/muon_shield", 2, GeoTrf::Translate3D(0.0, 0.0, 16763.3));

    // Build and place the Scattering and Neutrino Detector (SND).
    // Z: 26.40 to 31.50 m (WARM muon-shield configuration) → centre 28.95 m.
    // The SND sits within the downstream end of the muon-shield region, so its
    // envelope overlaps the muon-shield container by design (see test_consistency).
    NeutrinoDetectorFactory neutrinoDetectorFactory(materials);
    GeoPhysVol* neutrinoDetector = neutrinoDetectorFactory.build();
    placeChild(world, neutrinoDetector, "/SHiP/neutrino_detector", 9,
               GeoTrf::Translate3D(0.0, 0.0, 28.95 * 1000.0));

    // Build and place UpstreamTagger (sensitive scintillator slab)
    // Z: 32.52 to 32.92 m → centre: 32.72 m
    SHiPUBTManager ubtManager;
    UpstreamTaggerFactory upstreamTaggerFactory(materials);
    GeoVPhysVol* upstreamTagger = upstreamTaggerFactory.build(&ubtManager);
    placeChild(world, upstreamTagger, "/SHiP/upstream_tagger", 3,
               GeoTrf::Translate3D(0.0, 0.0, 32.72 * 1000.0));

    // Build and place DecayVolume
    // Z: 32.92 to 83.32 m → centre: 58.12 m
    DecayVolumeFactory decayVolumeFactory(materials);
    GeoPhysVol* decayVolume = decayVolumeFactory.build();
    placeChild(world, decayVolume, "/SHiP/decay_volume", 4,
               GeoTrf::Translate3D(0.0, 0.0, 58.12 * 1000.0));

    // Build and place Trackers (container with 4 stations).
    // The factory already handles internal positioning; place the container at
    // its centre Z (average of station 1 and 4 centres).
    TrackersFactory trackersFactory(materials);
    GeoPhysVol* trackers = trackersFactory.build();
    constexpr double trackersCentreZ = (84.07 + 95.07) / 2.0 * 1000.0;
    placeChild(world, trackers, "/SHiP/trackers", 5,
               GeoTrf::Translate3D(0.0, 0.0, trackersCentreZ));

    // Build and place Magnet
    // Z: 87.07 to 92.07 m → centre: 89.57 m
    MagnetFactory magnetFactory(materials);
    GeoPhysVol* magnet = magnetFactory.build();
    placeChild(world, magnet, "/SHiP/magnet", 6, GeoTrf::Translate3D(0.0, 0.0, 89.57 * 1000.0));

    // Build and place TimingDetector
    // Z: 95.902 m (from GDML reference)
    TimingDetectorFactory timingDetectorFactory(materials);
    GeoPhysVol* timingDetector = timingDetectorFactory.build();
    placeChild(world, timingDetector, "/SHiP/timing_detector", 7,
               GeoTrf::Translate3D(0.0, 0.0, 95.902 * 1000.0));

    // Build and place Calorimeter (ECAL + HCAL).
    // The layer structure is driven by calo.toml; the outer container dimensions
    // and placement are fixed to match the SHiP subsystem envelope.
    CalorimeterFactory calorimeterFactory(materials);
    GeoPhysVol* calorimeter = calorimeterFactory.build();
    placeChild(world, calorimeter, "/SHiP/calorimeter", 8,
               GeoTrf::Translate3D(0.0, 0.0, 98.32 * 1000.0));

    return world;
}

}  // namespace SHiPGeometry
