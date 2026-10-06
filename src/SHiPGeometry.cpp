// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include "Calorimeter/CalorimeterFactory.h"
#include "Cavern/CavernFactory.h"
#include "DecayVolume/DecayVolumeFactory.h"
#include "Magnet/MagnetFactory.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/Layout.h"
#include "SHiPGeometry/Placement.h"
#include "SHiPGeometry/SHiPMaterials.h"
#include "SHiPGeometry/Units.h"
#include "Target/TargetFactory.h"
#include "TimingDetector/TimingDetectorFactory.h"
#include "Trackers/TrackersFactory.h"
#include "UpstreamTagger/SHiPUBTManager.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoPhysVol.h>

#include <string>

namespace SHiPGeometry {

namespace {

/// Place a subsystem container at its slot in the layout.
void place(GeoVPhysVol* world, GeoVPhysVol* child, const Layout::Slot& slot) {
    using units::gm;
    placeChild(world, child, std::string(slot.path), slot.id,
               GeoTrf::Translate3D(gm(slot.x), gm(slot.y), gm(slot.z)));
}

}  // namespace

SHiPGeometryBuilder::SHiPGeometryBuilder() = default;
SHiPGeometryBuilder::~SHiPGeometryBuilder() = default;

GeoPhysVol* SHiPGeometryBuilder::build() {
    // Create central materials manager
    SHiPMaterials materials;

    // Build the cavern (world volume). Subsystem positions come from
    // Layout.h, which checks them against each other at compile time.
    CavernFactory cavernFactory(materials);
    GeoPhysVol* world = cavernFactory.build();

    // Build and place the Target
    TargetFactory targetFactory(materials);
    GeoPhysVol* target = targetFactory.build();

    place(world, target, Layout::kTarget);

    // Build and place MuonShieldArea
    MuonShieldFactory muonShieldFactory(materials);
    GeoPhysVol* muonShield = muonShieldFactory.build();
    place(world, muonShield, Layout::kMuonShield);

    // Build and place the Scattering and Neutrino Detector (SND)
    NeutrinoDetectorFactory neutrinoDetectorFactory(materials);
    GeoPhysVol* neutrinoDetector = neutrinoDetectorFactory.build();
    place(world, neutrinoDetector, Layout::kNeutrinoDetector);

    // Build and place UpstreamTagger (sensitive scintillator tiles)
    SHiPUBTManager ubtManager;
    UpstreamTaggerFactory upstreamTaggerFactory(materials);
    GeoVPhysVol* upstreamTagger = upstreamTaggerFactory.build(&ubtManager);
    place(world, upstreamTagger, Layout::kUpstreamTagger);

    // Build and place DecayVolume
    DecayVolumeFactory decayVolumeFactory(materials);
    GeoPhysVol* decayVolume = decayVolumeFactory.build();
    place(world, decayVolume, Layout::kDecayVolume);

    // Build and place Trackers (container with 4 stations)
    TrackersFactory trackersFactory(materials);
    GeoPhysVol* trackers = trackersFactory.build();
    place(world, trackers, Layout::kTrackers);

    // Build and place Magnet
    MagnetFactory magnetFactory(materials);
    GeoPhysVol* magnet = magnetFactory.build();
    place(world, magnet, Layout::kMagnet);

    // Build and place TimingDetector
    TimingDetectorFactory timingDetectorFactory(materials);
    GeoPhysVol* timingDetector = timingDetectorFactory.build();
    place(world, timingDetector, Layout::kTimingDetector);

    // Build and place Calorimeter (ECAL + HCAL)
    CalorimeterFactory calorimeterFactory(materials);
    GeoPhysVol* calorimeter = calorimeterFactory.build();
    place(world, calorimeter, Layout::kCalorimeter);

    return world;
}

}  // namespace SHiPGeometry
