// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Trackers/TrackersFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>

#include <array>
#include <numbers>
#include <string>

namespace SHiPGeometry {

namespace {

// ── Internal layout helpers (private to this TU) ─────────────────────────────
//
// The straw pattern fills a (s_apertureX x s_apertureY) rectangle. Around it
// sits a hollow material frame; outside that, an air "layer envelope" that
// gets stereo-rotated by the parent station. All numbers are in millimetres.
//
// Aperture clearance: extra space around the straw pattern so that the
// sub-layer envelopes — which extend by +/- one straw radius in Z and (for
// the staggered sub-layer) +1/2 pitch in Y — fit inside the frame without
// touching its walls.
constexpr double kApClearX = 5.0;
constexpr double kApClearY = 15.0;

// Small slack between the layer envelope and the frame outer surface (and
// between the sub-layer envelope and the frame aperture). Without this,
// CheckOverlaps in Geant4 flags cosmetic touches.
constexpr double kEnvClearance = 5.0;

// Per-straw additional Z separation between the two sub-layer envelopes so
// they do not share a face at z = 0 in the layer frame.
constexpr double kSubLayerZSlack = 0.55;

// Frame aperture (inner hole) half-sizes in mm.
constexpr double kApHalfX = TrackersFactory::s_apertureX / 2.0 + kApClearX;   // 2005
constexpr double kApHalfY = TrackersFactory::s_apertureY / 2.0 + kApClearY;   // 3015

// Frame outer half-sizes in mm.
constexpr double kFrHalfX = kApHalfX + TrackersFactory::s_frameWidthX;        // 2105
constexpr double kFrHalfY = kApHalfY + TrackersFactory::s_frameWidthY;        // 3115

// Layer envelope half-sizes in mm.
constexpr double kLayHalfX = kFrHalfX + kEnvClearance;                        // 2110
constexpr double kLayHalfY = kFrHalfY + kEnvClearance;                        // 3120
constexpr double kLayHalfZ = TrackersFactory::s_frameHalfZ + kEnvClearance;   //   27

// Sub-layer envelope half-sizes in mm. Slightly smaller than the aperture so
// it sits cleanly inside the frame; thick enough in Z to wrap one straw.
constexpr double kFrameClearance = 0.5;
constexpr double kSlHalfX = kApHalfX - kFrameClearance;                       // 2004.5
constexpr double kSlHalfY = kApHalfY - kFrameClearance;                       // 3014.5
constexpr double kSlHalfZ = TrackersFactory::s_strawRadius + 0.5;             //   10.5

// Z stack of layers within a station: kNLayers air slabs of half-thickness
// kLayHalfZ separated by a small gap.
constexpr double kLayerGap   = 5.0;
constexpr double kLayerPitch = 2.0 * kLayHalfZ + kLayerGap;                   // 59

double signedStereoDeg(int layerIndex) {
    // Layers 0,2 -> +angle (u view); layers 1,3 -> -angle (v view).
    return (layerIndex % 2 == 0 ? +1.0 : -1.0) * TrackersFactory::s_stereoAngleDeg;
}

double layerZInStation(int layerIndex) {
    // Centre the 4-layer stack on z = 0 within the station envelope.
    return -0.5 * (TrackersFactory::s_nLayers - 1) * kLayerPitch
           + layerIndex * kLayerPitch;
}

}  // namespace

TrackersFactory::TrackersFactory(SHiPMaterials& materials) : m_materials(materials) {}

// ─────────────────────────────────────────────────────────────────────────────
// build()
// ─────────────────────────────────────────────────────────────────────────────
GeoPhysVol* TrackersFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    buildSharedLogVols();

    // Tracker container that spans all 4 stations. Half-Z is large enough to
    // include the most upstream and downstream station envelopes.
    auto* containerBox  = new GeoBox(s_halfX, s_halfY, s_containerHalfZ);
    auto* containerLog  = new GeoLogVol("/SHiP/trackers", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    constexpr std::array<double, 4> stationZ = {s_station1Z, s_station2Z,
                                                s_station3Z, s_station4Z};

    for (int i = 0; i < s_nStations; ++i) {
        GeoPhysVol* stationPhys = buildStation(i);

        const std::string stationName = "/SHiP/trackers/station_" + std::to_string(i + 1);
        const double      relativeZ   = stationZ[i] - s_containerCentreZ;

        containerPhys->add(new GeoNameTag(stationName));
        containerPhys->add(new GeoIdentifierTag(i + 1));
        containerPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, relativeZ)));
        containerPhys->add(stationPhys);
    }

    return containerPhys;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildSharedLogVols()
//
// Builds every LogVol that is repeated across stations exactly once, so the
// final tree contains O(few) LogVols instead of O(9600). Placement-time
// individuation is done with GeoNameTag and GeoIdentifierTag.
// ─────────────────────────────────────────────────────────────────────────────
void TrackersFactory::buildSharedLogVols() {
    const GeoMaterial* air      = m_materials.requireMaterial("Air");
    const GeoMaterial* mylar    = m_materials.requireMaterial("Mylar");
    const GeoMaterial* gasArCO2 = m_materials.requireMaterial("ArCO2_70_30");
    const GeoMaterial* alu      = m_materials.requireMaterial("Aluminium");

    // ── Straw wall + gas (one pair shared by every straw in every layer) ────
    // The wall is built as a SOLID Mylar tube; the gas tube sits inside it as
    // a daughter and physics-wise replaces the wall material in its volume.
    // This avoids the mother/daughter overlap that the (rMin, rMax) hollow
    // tube + gas-cylinder combination triggers in Geant4's overlap checker.
    const double rGas  = s_strawRadius - s_wallThick;
    const double rWall = s_strawRadius;
    const double half  = 0.5 * s_strawLength;

    auto* wallTube = new GeoTube(0.0, rWall, half);
    m_strawWallLog = new GeoLogVol("StrawWall", wallTube, mylar);

    auto* gasTube  = new GeoTube(0.0, rGas, half);
    m_strawGasLog  = new GeoLogVol("StrawGas", gasTube, gasArCO2);

    // ── Sub-layer envelopes (nominal + shifted) ─────────────────────────────
    auto* slBox        = new GeoBox(kSlHalfX, kSlHalfY, kSlHalfZ);
    m_subLayerNominal  = new GeoLogVol("StrawSubLayer_nominal", slBox, air);
    m_subLayerShifted  = new GeoLogVol("StrawSubLayer_shifted", slBox, air);

    // ── Layer envelope (one shape, reused for all 16 placements) ────────────
    auto* layBox = new GeoBox(kLayHalfX, kLayHalfY, kLayHalfZ);
    m_layerLog   = new GeoLogVol("StrawLayer", layBox, air);

    // ── Material frame: outer rectangle minus inner aperture ────────────────
    auto* outerBox   = new GeoBox(kFrHalfX, kFrHalfY, s_frameHalfZ);
    auto* innerBox   = new GeoBox(kApHalfX, kApHalfY, s_frameHalfZ + 1.0);
    auto* frameShape = new GeoShapeSubtraction(outerBox, innerBox);
    m_frameLog       = new GeoLogVol("StrawFrame", frameShape, alu);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildStation()
// ─────────────────────────────────────────────────────────────────────────────
GeoPhysVol* TrackersFactory::buildStation(int stationIndex) {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Each station uses the per-station envelope from subsystem_envelopes.csv.
    // The internal stack (4 layers, ~125 mm half-Z) sits comfortably inside
    // the 500 mm half-Z envelope — leaving room for service material later.
    // The station LogVol name is unique per station so test_trackers' lookup
    // by /SHiP/trackers/station_<n> continues to work.
    const std::string stationName =
        "/SHiP/trackers/station_" + std::to_string(stationIndex + 1);

    auto* stationBox  = new GeoBox(s_halfX, s_halfY, s_halfZ);
    auto* stationLog  = new GeoLogVol(stationName, stationBox, air);
    auto* stationPhys = new GeoPhysVol(stationLog);

    for (int j = 0; j < s_nLayers; ++j) {
        placeLayer(stationPhys, j, signedStereoDeg(j));
    }
    return stationPhys;
}

// ─────────────────────────────────────────────────────────────────────────────
// placeLayer()
// ─────────────────────────────────────────────────────────────────────────────
void TrackersFactory::placeLayer(GeoPhysVol* station, int layerIndex,
                                 double signedAngleDeg) const {
    auto* layerPhys = new GeoPhysVol(m_layerLog);

    // ── 1. Frame at z = 0 (rotates with the view) ───────────────────────────
    layerPhys->add(new GeoNameTag("StrawFrame"));
    layerPhys->add(new GeoIdentifierTag(100 + layerIndex));
    layerPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    layerPhys->add(new GeoPhysVol(m_frameLog));

    // ── 2. Two sub-layers, staggered in Y and offset in Z ───────────────────
    const double dz = s_strawRadius + kSubLayerZSlack;  // ~10.55 mm

    layerPhys->add(new GeoNameTag("StrawSubLayer_0"));
    layerPhys->add(new GeoIdentifierTag(0));
    layerPhys->add(new GeoTransform(GeoTrf::TranslateZ3D(-dz)));
    {
        auto* sl0 = new GeoPhysVol(m_subLayerNominal);
        placeSubLayer(sl0, /*shifted=*/false);
        layerPhys->add(sl0);
    }

    layerPhys->add(new GeoNameTag("StrawSubLayer_1"));
    layerPhys->add(new GeoIdentifierTag(1));
    layerPhys->add(new GeoTransform(GeoTrf::TranslateZ3D(+dz)));
    {
        auto* sl1 = new GeoPhysVol(m_subLayerShifted);
        placeSubLayer(sl1, /*shifted=*/true);
        layerPhys->add(sl1);
    }

    // ── 3. Place the (now populated) layer envelope inside the station ─────
    const double angleRad = signedAngleDeg * std::numbers::pi / 180.0;
    const double zLay     = layerZInStation(layerIndex);

    GeoTrf::Transform3D xfLayer =
        GeoTrf::TranslateZ3D(zLay) * GeoTrf::RotateZ3D(angleRad);

    station->add(new GeoNameTag("Layer_" + std::to_string(layerIndex)));
    station->add(new GeoIdentifierTag(layerIndex));
    station->add(new GeoTransform(xfLayer));
    station->add(layerPhys);
}

// ─────────────────────────────────────────────────────────────────────────────
// placeSubLayer()
//
// Lays down s_nStraws straws along Y, pitch = 2 * straw radius. Each straw is
// a Mylar wall PhysVol with a gas PhysVol daughter; both LogVols are shared
// across every straw in the geometry. The straw axis is along the layer's
// local X (achieved by a +90 deg rotation about Y from the GeoTube's native
// Z axis).
// ─────────────────────────────────────────────────────────────────────────────
void TrackersFactory::placeSubLayer(GeoPhysVol* layer, bool shifted) const {
    const double pitch  = 2.0 * s_strawRadius;
    const double yStart = -(s_nStraws - 1) * 0.5 * pitch;
    const double yShift = shifted ? s_strawRadius : 0.0;

    for (int i = 0; i < s_nStraws; ++i) {
        const double yStraw = yStart + i * pitch + yShift;

        // The wall PhysVol carries the gas as its single daughter. Constructing
        // it here per straw is necessary because each placement gets its own
        // GeoPhysVol node — but the underlying LogVols are shared.
        auto* wallPhys = new GeoPhysVol(m_strawWallLog);
        wallPhys->add(new GeoNameTag("StrawGas"));
        wallPhys->add(new GeoPhysVol(m_strawGasLog));

        layer->add(new GeoNameTag("Straw"));
        layer->add(new GeoIdentifierTag(i));
        layer->add(new GeoTransform(GeoTrf::TranslateY3D(yStraw) *
                                    GeoTrf::RotateY3D(std::numbers::pi / 2.0)));
        layer->add(wallPhys);
    }
}

}  // namespace SHiPGeometry
