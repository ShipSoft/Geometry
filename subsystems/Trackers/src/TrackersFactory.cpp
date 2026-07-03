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
#include <GeoModelKernel/Units.h>

#include <cmath>
#include <string>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

// ── file-scope geometry helpers ──────────────────────────────────────────────
namespace {

// Clearances (mm). The view aperture is a little larger than the nominal
// straw pattern so the staggered sub-layer and the straw outer radius fit
// inside; the view envelope is a little larger than the frame outer box.
constexpr double kApertureClearX = 5.0;
constexpr double kApertureClearY = 15.0;
constexpr double kEnvClearance = 5.0;

// View aperture (inner frame hole) half-sizes.
constexpr double kApertureHalfX = TrackersFactory::s_apertureX / 2.0 + kApertureClearX;  // 2005
constexpr double kApertureHalfY = TrackersFactory::s_apertureY / 2.0 + kApertureClearY;  // 3015

// Frame outer half-sizes.
constexpr double kFrameHalfX = kApertureHalfX + TrackersFactory::s_frameBarX;  // 2105
constexpr double kFrameHalfY = kApertureHalfY + TrackersFactory::s_frameBarY;  // 3115

// View envelope half-sizes.
constexpr double kViewHalfX = kFrameHalfX + kEnvClearance;                    // 2110
constexpr double kViewHalfY = kFrameHalfY + kEnvClearance;                    // 3120
constexpr double kViewHalfZ = TrackersFactory::s_frameHalfZ + kEnvClearance;  // 27

// Small gap between the frame aperture and the sub-layer envelope.
constexpr double kFrameClearance = 0.5;

// Signed stereo angle for a view: views 0,2 → +, views 1,3 → -.
double stereoSignedDeg(int viewIndex) {
    const double sign = (viewIndex % 2 == 0) ? +1.0 : -1.0;
    return sign * TrackersFactory::s_stereoAngleDeg;
}

}  // namespace

// ── constructor ──────────────────────────────────────────────────────────────

TrackersFactory::TrackersFactory(SHiPMaterials& materials) : m_materials(materials) {}

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Container volume spanning all 4 stations.
    auto* containerBox = new GeoBox(s_halfX, s_halfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/trackers", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    const std::array<double, s_nStations> stationZ = {s_station1Z, s_station2Z, s_station3Z,
                                                      s_station4Z};

    for (int i = 0; i < s_nStations; ++i) {
        GeoPhysVol* stationPhys = buildStation(i);

        // Place the station relative to the container centre.
        const double relativeZ = stationZ[i] - s_containerCentreZ;
        const GeoTrf::Transform3D stationTrf = GeoTrf::Translate3D(0.0, 0.0, relativeZ);

        // Name kept as "/SHiP/trackers/station_<n>" for downstream lookups.
        const std::string stationName = "/SHiP/trackers/station_" + std::to_string(i + 1);
        containerPhys->add(new GeoNameTag(stationName));
        containerPhys->add(new GeoIdentifierTag(i));
        containerPhys->add(new GeoTransform(stationTrf));
        containerPhys->add(stationPhys);
    }

    // Inert tracker-magnet marker, in the gap between station 2 and the
    // spectrometer-magnet yoke (see buildTrackerMagnet / s_trackerMagnetZ).
    {
        GeoPhysVol* trackerMagnet = buildTrackerMagnet();
        const double relativeZ = s_trackerMagnetZ - s_containerCentreZ;
        containerPhys->add(new GeoNameTag("/SHiP/trackers/tracker_magnet"));
        containerPhys->add(new GeoIdentifierTag(s_nStations));
        containerPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, relativeZ)));
        containerPhys->add(trackerMagnet);
    }

    return containerPhys;
}

// ── buildStation ─────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildStation(int stationIndex) {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Station envelope: fixed GDML statbox size. Kept exactly so the geometry
    // consistency test (station box <= 3000 x 3500 x 500 mm) still passes.
    const std::string stationName = "/SHiP/trackers/station_" + std::to_string(stationIndex + 1);
    auto* stationBox = new GeoBox(s_halfX, s_halfY, s_halfZ);
    auto* stationLog = new GeoLogVol(stationName, stationBox, air);
    auto* stationPhys = new GeoPhysVol(stationLog);

    // Four stereo views, stacked along Z within the station, each rotated
    // about the beam axis by its signed stereo angle.
    const double viewGap = 5.0;
    const double viewPitch = 2.0 * kViewHalfZ + viewGap;

    for (int v = 0; v < s_nViews; ++v) {
        GeoPhysVol* viewPhys = buildView(stationIndex, v);

        const double zView = -0.5 * (s_nViews - 1) * viewPitch + v * viewPitch;
        const double angleRad = stereoSignedDeg(v) * deg;
        const GeoTrf::Transform3D viewTrf =
            GeoTrf::Translate3D(0.0, 0.0, zView) * GeoTrf::RotateZ3D(angleRad);

        const std::string viewName = stationName + "/view_" + std::to_string(v);
        stationPhys->add(new GeoNameTag(viewName));
        stationPhys->add(new GeoIdentifierTag(v));
        stationPhys->add(new GeoTransform(viewTrf));
        stationPhys->add(viewPhys);
    }

    return stationPhys;
}

// ── buildView ────────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildView(int stationIndex, int viewIndex) {
    // The stereo rotation is applied by the parent station; the view is built
    // unrotated here. It holds a material frame plus two staggered sub-layers.
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    const std::string viewName = "/SHiP/trackers/station_" + std::to_string(stationIndex + 1) +
                                 "/view_" + std::to_string(viewIndex);

    auto* viewBox = new GeoBox(kViewHalfX, kViewHalfY, kViewHalfZ);
    auto* viewLog = new GeoLogVol(viewName + "/envelope", viewBox, air);
    auto* viewPhys = new GeoPhysVol(viewLog);

    // Material frame.
    {
        GeoPhysVol* framePhys = buildFrame(stationIndex, viewIndex);
        viewPhys->add(new GeoNameTag(viewName + "/frame"));
        viewPhys->add(new GeoIdentifierTag(0));
        viewPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
        viewPhys->add(framePhys);
    }

    // Two sub-layers of straws. Centres at z = ±(strawRadius + 0.55) mm so the
    // two sub-layer envelopes do not overlap each other at z = 0.
    const double dz = s_strawRadius + 0.55;

    {
        GeoPhysVol* sub0 = buildSubLayer(stationIndex, viewIndex, false);
        viewPhys->add(new GeoNameTag(viewName + "/sublayer_0"));
        viewPhys->add(new GeoIdentifierTag(0));
        viewPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, -dz)));
        viewPhys->add(sub0);
    }
    {
        GeoPhysVol* sub1 = buildSubLayer(stationIndex, viewIndex, true);
        viewPhys->add(new GeoNameTag(viewName + "/sublayer_1"));
        viewPhys->add(new GeoIdentifierTag(1));
        viewPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, +dz)));
        viewPhys->add(sub1);
    }

    return viewPhys;
}

// ── buildFrame ───────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildFrame(int stationIndex, int viewIndex) {
    const GeoMaterial* frameMat = m_materials.requireMaterial(m_frameMaterialName);

    // Hollow rectangle = outer box minus aperture box. The inner box is made
    // slightly thicker in Z so the subtraction punches cleanly through.
    auto* outerBox = new GeoBox(kFrameHalfX, kFrameHalfY, s_frameHalfZ);
    auto* innerBox = new GeoBox(kApertureHalfX, kApertureHalfY, s_frameHalfZ + 1.0);
    auto* frameShape = new GeoShapeSubtraction(outerBox, innerBox);

    const std::string frameName = "/SHiP/trackers/station_" + std::to_string(stationIndex + 1) +
                                  "/view_" + std::to_string(viewIndex) + "/frame_body";
    auto* frameLog = new GeoLogVol(frameName, frameShape, frameMat);
    auto* framePhys = new GeoPhysVol(frameLog);

    return framePhys;
}

// ── buildSubLayer ────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildSubLayer(int stationIndex, int viewIndex, bool shifted) {
    // A sub-layer is a thin air slab holding s_nStraws straws. Both the
    // nominal and the shifted sub-layer share the same symmetric XY footprint;
    // the half-pitch Y stagger is applied here, per straw, via `shifted`.
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    const double pitch = 2.0 * s_strawRadius;               // 20 mm
    const double yStagger = shifted ? s_strawRadius : 0.0;  // +10 mm if shifted

    const double subHalfX = kApertureHalfX - kFrameClearance;
    const double subHalfY = kApertureHalfY - kFrameClearance;
    const double subHalfZ = s_strawRadius + 0.5;

    const std::string subName = "/SHiP/trackers/station_" + std::to_string(stationIndex + 1) +
                                "/view_" + std::to_string(viewIndex) + "/sublayer_" +
                                (shifted ? "1" : "0") + "_body";
    auto* subBox = new GeoBox(subHalfX, subHalfY, subHalfZ);
    auto* subLog = new GeoLogVol(subName, subBox, air);
    auto* subPhys = new GeoPhysVol(subLog);

    // Globally-unique straw id seed: keeps every straw log-volume name unique
    // across the whole subsystem (station, view, sub-layer, straw).
    const int subLayerOrdinal =
        ((stationIndex * s_nViews) + viewIndex) * s_nSubLayers + (shifted ? 1 : 0);
    int strawUid = subLayerOrdinal * s_nStraws;

    const double yStart = -(s_nStraws - 1) * 0.5 * pitch;

    for (int i = 0; i < s_nStraws; ++i) {
        const double yStraw = yStart + i * pitch + yStagger;

        // GeoTube axis is local Z; rotate it to lie along X (the straw axis).
        const GeoTrf::Transform3D strawTrf =
            GeoTrf::Translate3D(0.0, yStraw, 0.0) * GeoTrf::RotateY3D(90.0 * deg);

        subPhys->add(new GeoNameTag(subName + "/straw_" + std::to_string(i)));
        subPhys->add(new GeoIdentifierTag(i));
        subPhys->add(new GeoTransform(strawTrf));
        subPhys->add(buildStraw(strawUid++));
    }

    return subPhys;
}

// ── buildStraw ───────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildStraw(int uid) {
    // A straw is a solid Mylar cylinder (the wall) with a gas daughter that
    // fills the interior. Modelling the wall as a solid cylinder rather than a
    // hollow tube keeps the gas fully contained, with no mother-daughter
    // overlap. GeoTube axis is local Z; the parent sub-layer rotates it to X.
    const GeoMaterial* mylar = m_materials.requireMaterial("Mylar");
    const GeoMaterial* gas = m_materials.requireMaterial("ArCO2_70_30");

    const double rGas = s_strawRadius - s_wallThickness;
    const double rWall = s_strawRadius;
    const double halfLength = s_strawLength / 2.0;

    auto* wallTube = new GeoTube(0.0, rWall, halfLength);
    auto* wallLog =
        new GeoLogVol("/SHiP/trackers/straw_wall_" + std::to_string(uid), wallTube, mylar);
    auto* wallPhys = new GeoPhysVol(wallLog);

    auto* gasTube = new GeoTube(0.0, rGas, halfLength);
    auto* gasLog = new GeoLogVol("/SHiP/trackers/straw_gas_" + std::to_string(uid), gasTube, gas);
    auto* gasPhys = new GeoPhysVol(gasLog);

    wallPhys->add(new GeoNameTag("/SHiP/trackers/straw_gas"));
    wallPhys->add(new GeoIdentifierTag(0));
    wallPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    wallPhys->add(gasPhys);

    return wallPhys;
}

// ── buildTrackerMagnet ───────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildTrackerMagnet() {
    // Inert, air-filled marker for the tracker magnet. It is deliberately
    // sized to fit the ~0.5 m gap between station 2 and the spectrometer
    // magnet yoke, so it does not overlap the separate Magnet subsystem.
    // It is NOT a physically-scaled dipole — it gives the tracker magnet a
    // named placeholder volume that simulation/field code can locate by the
    // log-volume name "/SHiP/trackers/tracker_magnet".
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    auto* box = new GeoBox(s_halfX, s_halfY, s_trackerMagnetHalfZ);
    auto* log = new GeoLogVol("/SHiP/trackers/tracker_magnet", box, air);
    auto* phys = new GeoPhysVol(log);

    return phys;
}

}  // namespace SHiPGeometry
