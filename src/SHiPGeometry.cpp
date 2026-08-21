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
#include "NeutrinoDetector/SNDEnvelope.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/Units.h>

namespace SHiPGeometry {

SHiPGeometryBuilder::SHiPGeometryBuilder() = default;
SHiPGeometryBuilder::~SHiPGeometryBuilder() = default;

GeoPhysVol* SHiPGeometryBuilder::build() {
    // Unit shorthands (GeoModel's native length unit is mm)
    constexpr double mm = GeoModelKernelUnits::mm;
    constexpr double cm = GeoModelKernelUnits::cm;
    constexpr double m = GeoModelKernelUnits::m;

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
    placeChild(world, target, "/SHiP/target", 1, GeoTrf::Translate3D(0.0, -14.45 * cm, 43.25 * cm));

    // Build the muon shield, with the neutrino detector embedded inside it.
    //
    // The SND is an independent subsystem, but in volume terms it is a daughter
    // of the muon-shield container. Its footprint is declared in SD.toml: we
    // reserve that box in the shield iron (carved by Boolean subtraction in
    // build()) and then nest the detector in the resulting cavity. Both the
    // reservation and the placement come from the same envelope, so the SND
    // position is defined in exactly one place.
    MuonShieldFactory muonShieldFactory(materials);

    const SNDEnvelope sndEnvelope = readSNDEnvelope();
    muonShieldFactory.reserveSpace(sndEnvelope.centre_mm, sndEnvelope.size_mm,
                                   sndEnvelope.rotation_deg);

    NeutrinoDetectorFactory neutrinoDetectorFactory(materials);
    GeoPhysVol* neutrinoDetector = neutrinoDetectorFactory.build();
    muonShieldFactory.embedDaughter(neutrinoDetector, sndEnvelope.centre_mm[2],
                                    "/SHiP/neutrino_detector");

    // The container is built centred on its own origin, so it is placed at the
    // envelope centre reported by the factory after build().
    GeoPhysVol* muonShield = muonShieldFactory.build();
    placeChild(world, muonShield, "/SHiP/muon_shield", 2,
               GeoTrf::Translate3D(0.0, 0.0, muonShieldFactory.centreZ_mm()));
    // Build and place UpstreamTagger (sensitive scintillator slab)
    // Z: 32.52 to 32.92 m → centre: 32.72 m
    SHiPUBTManager ubtManager;
    UpstreamTaggerFactory upstreamTaggerFactory(materials);
    GeoVPhysVol* upstreamTagger = upstreamTaggerFactory.build(&ubtManager);
    placeChild(world, upstreamTagger, "/SHiP/upstream_tagger", 3,
               GeoTrf::Translate3D(0.0, 0.0, 32.72 * m));

    // Build and place DecayVolume
    // Z: 32.92 to 83.32 m → centre: 58.12 m
    DecayVolumeFactory decayVolumeFactory(materials);
    GeoPhysVol* decayVolume = decayVolumeFactory.build();
    placeChild(world, decayVolume, "/SHiP/decay_volume", 4,
               GeoTrf::Translate3D(0.0, 0.0, 58.12 * m));

    // Build and place Trackers (container with 4 stations).
    // The factory already handles internal positioning; place the container at
    // its centre Z (average of station 1 and 4 centres).
    TrackersFactory trackersFactory(materials);
    GeoPhysVol* trackers = trackersFactory.build();
    constexpr double trackersCentreZ = (84.07 + 95.07) / 2.0 * m;
    placeChild(world, trackers, "/SHiP/trackers", 5,
               GeoTrf::Translate3D(0.0, 0.0, trackersCentreZ));

    // Build and place Magnet
    // Z: 87.07 to 92.07 m → centre: 89.57 m
    MagnetFactory magnetFactory(materials);
    GeoPhysVol* magnet = magnetFactory.build();
    placeChild(world, magnet, "/SHiP/magnet", 6, GeoTrf::Translate3D(0.0, 0.0, 89.57 * m));

    // Build and place TimingDetector
    // Z: 95.902 m (from GDML reference)
    TimingDetectorFactory timingDetectorFactory(materials);
    GeoPhysVol* timingDetector = timingDetectorFactory.build();
    placeChild(world, timingDetector, "/SHiP/timing_detector", 7,
               GeoTrf::Translate3D(0.0, 0.0, 95.902 * m));

    // Build and place Calorimeter (ECAL + HCAL).
    // The layer structure is driven by calo.toml; the outer container dimensions
    // and placement are fixed to match the SHiP subsystem envelope.
    CalorimeterFactory calorimeterFactory(materials);
    GeoPhysVol* calorimeter = calorimeterFactory.build();
    placeChild(world, calorimeter, "/SHiP/calorimeter", 8,
               GeoTrf::Translate3D(0.0, 0.0, 98.32 * m));

    return world;
}

}  // namespace SHiPGeometry
