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
#include <GeoModelKernel/GeoSerialDenominator.h>
#include <GeoModelKernel/GeoSerialIdentifier.h>
#include <GeoModelKernel/GeoSerialTransformer.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoXF.h>

#include <GeoGenericFunctions/Variable.h>
#include <cmath>
#include <string>

namespace SHiPGeometry {

using units::gm;
using units::mm;

// ── file-scope geometry helpers ──────────────────────────────────────────────
namespace {

// Clearances. The view aperture is a little larger than the nominal
// straw pattern so the staggered sub-layer and the straw outer radius fit
// inside; the view envelope is a little larger than the frame outer box.
constexpr auto kApertureClearX = 5.0 * mm;
constexpr auto kApertureClearY = 15.0 * mm;
constexpr auto kEnvClearance = 5.0 * mm;

// View aperture (inner frame hole) half-sizes.
constexpr auto kApertureHalfX = TrackersFactory::s_apertureX / 2.0 + kApertureClearX;  // 2005
constexpr auto kApertureHalfY = TrackersFactory::s_apertureY / 2.0 + kApertureClearY;  // 3015

// Frame outer half-sizes.
constexpr auto kFrameHalfX = kApertureHalfX + TrackersFactory::s_frameBarX;  // 2105
constexpr auto kFrameHalfY = kApertureHalfY + TrackersFactory::s_frameBarY;  // 3115

// View envelope half-sizes.
constexpr auto kViewHalfX = kFrameHalfX + kEnvClearance;                    // 2110
constexpr auto kViewHalfY = kFrameHalfY + kEnvClearance;                    // 3120
constexpr auto kViewHalfZ = TrackersFactory::s_frameHalfZ + kEnvClearance;  // 27

// Small gap between the frame aperture and the sub-layer envelope.
constexpr auto kFrameClearance = 0.5 * mm;

// Signed stereo angle for a view: views 0,2 → +, views 1,3 → -.
units::AngleDeg stereoSignedDeg(int viewIndex) {
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
    auto* containerBox = new GeoBox(gm(s_halfX), gm(s_halfY), gm(s_containerHalfZ));
    auto* containerLog = new GeoLogVol("/SHiP/trackers", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    const std::array<units::LengthMm, s_nStations> stationZ = {s_station1Z, s_station2Z,
                                                               s_station3Z, s_station4Z};

    for (int i = 0; i < s_nStations; ++i) {
        GeoPhysVol* stationPhys = buildStation(i);

        // Place the station relative to the container centre.
        const double relativeZ = gm(stationZ[i] - s_containerCentreZ);
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
        const double relativeZ = gm(s_trackerMagnetZ - s_containerCentreZ);
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
    auto* stationBox = new GeoBox(gm(s_halfX), gm(s_halfY), gm(s_halfZ));
    auto* stationLog = new GeoLogVol(stationName, stationBox, air);
    auto* stationPhys = new GeoPhysVol(stationLog);

    // Four stereo views, stacked along Z within the station, each rotated
    // about the beam axis by its signed stereo angle.
    const auto viewGap = 5.0 * mm;
    const auto viewPitch = 2.0 * kViewHalfZ + viewGap;

    for (int v = 0; v < s_nViews; ++v) {
        GeoPhysVol* viewPhys = buildView(stationIndex, v);

        const auto zView = -0.5 * (s_nViews - 1) * viewPitch + v * viewPitch;
        const double angleRad = gm(stereoSignedDeg(v));
        const GeoTrf::Transform3D viewTrf =
            GeoTrf::Translate3D(0.0, 0.0, gm(zView)) * GeoTrf::RotateZ3D(angleRad);

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

    auto* viewBox = new GeoBox(gm(kViewHalfX), gm(kViewHalfY), gm(kViewHalfZ));
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
    const auto dz = s_strawRadius + 0.55 * mm;

    {
        GeoPhysVol* sub0 = buildSubLayer(stationIndex, viewIndex, false);
        viewPhys->add(new GeoNameTag(viewName + "/sublayer_0"));
        viewPhys->add(new GeoIdentifierTag(0));
        viewPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, gm(-dz))));
        viewPhys->add(sub0);
    }
    {
        GeoPhysVol* sub1 = buildSubLayer(stationIndex, viewIndex, true);
        viewPhys->add(new GeoNameTag(viewName + "/sublayer_1"));
        viewPhys->add(new GeoIdentifierTag(1));
        viewPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, gm(+dz))));
        viewPhys->add(sub1);
    }

    return viewPhys;
}

// ── buildFrame ───────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildFrame(int stationIndex, int viewIndex) {
    const GeoMaterial* frameMat = m_materials.requireMaterial(m_frameMaterialName);

    // Hollow rectangle = outer box minus aperture box. The inner box is made
    // slightly thicker in Z so the subtraction punches cleanly through.
    auto* outerBox = new GeoBox(gm(kFrameHalfX), gm(kFrameHalfY), gm(s_frameHalfZ));
    auto* innerBox =
        new GeoBox(gm(kApertureHalfX), gm(kApertureHalfY), gm(s_frameHalfZ + 1.0 * mm));
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

    const auto pitch = 2.0 * s_strawRadius;                    // 20 mm
    const auto yStagger = shifted ? s_strawRadius : 0.0 * mm;  // +10 mm if shifted

    const auto subHalfX = kApertureHalfX - kFrameClearance;
    const auto subHalfY = kApertureHalfY - kFrameClearance;
    const auto subHalfZ = s_strawRadius + 0.5 * mm;

    const std::string subName = "/SHiP/trackers/station_" + std::to_string(stationIndex + 1) +
                                "/view_" + std::to_string(viewIndex) + "/sublayer_" +
                                (shifted ? "1" : "0") + "_body";
    auto* subBox = new GeoBox(gm(subHalfX), gm(subHalfY), gm(subHalfZ));
    auto* subLog = new GeoLogVol(subName, subBox, air);
    auto* subPhys = new GeoPhysVol(subLog);

    // Globally-unique straw id seed: the GeoSerialIdentifier base so every
    // straw keeps a unique id across the whole subsystem (station, view,
    // sub-layer, straw).
    const int subLayerOrdinal =
        ((stationIndex * s_nViews) + viewIndex) * s_nSubLayers + (shifted ? 1 : 0);
    const int strawUid = subLayerOrdinal * s_nStraws;

    const auto yStart = -(s_nStraws - 1) * 0.5 * pitch;

    // Every straw is geometrically identical, so one shared straw subtree is
    // placed s_nStraws times along Y by a GeoSerialTransformer: O(s_nStraws)
    // tree nodes and per-straw log volumes collapse to one straw and one node.
    // Pow(TranslateY(1 mm), yStraw) advances each copy in Y; RotateY(90°) lays
    // the GeoTube (local Z) axis along X. GeoSerialIdentifier reproduces the
    // per-straw ids (base = subLayerOrdinal · s_nStraws); GeoSerialDenominator
    // reproduces the "<subName>/straw_<i>" names.
    GeoGenfun::Variable i;
    GeoGenfun::GENFUNCTION yStraw = gm(yStart + yStagger) + gm(pitch) * i;
    GeoXF::TRANSFUNCTION strawXF =
        GeoXF::Pow(GeoTrf::Transform3D(GeoTrf::Translate3D(0.0, gm(1.0 * mm), 0.0)), yStraw) *
        GeoTrf::Transform3D(GeoTrf::RotateY3D(gm(90.0 * units::deg)));

    subPhys->add(new GeoSerialDenominator(subName + "/straw_"));
    subPhys->add(new GeoSerialIdentifier(strawUid));
    subPhys->add(new GeoSerialTransformer(buildStraw(), &strawXF, s_nStraws));

    return subPhys;
}

// ── buildStraw ───────────────────────────────────────────────────────────────

GeoPhysVol* TrackersFactory::buildStraw() {
    // A straw is a solid Mylar cylinder (the wall) with a gas daughter that
    // fills the interior. Modelling the wall as a solid cylinder rather than a
    // hollow tube keeps the gas fully contained, with no mother-daughter
    // overlap. GeoTube axis is local Z; the parent sub-layer rotates it to X.
    //
    // Every straw in every sub-layer is identical, so the subtree (with its two
    // shared log volumes and shapes) is built once and reused; sub-layers place
    // it via GeoSerialTransformer.
    if (m_strawPhys)
        return m_strawPhys;

    const GeoMaterial* mylar = m_materials.requireMaterial("Mylar");
    const GeoMaterial* gas = m_materials.requireMaterial("ArCO2_70_30");

    const auto rGas = s_strawRadius - s_wallThickness;
    const auto rWall = s_strawRadius;
    const auto halfLength = s_strawLength / 2.0;

    auto* wallTube = new GeoTube(0.0, gm(rWall), gm(halfLength));
    auto* wallLog = new GeoLogVol("/SHiP/trackers/straw_wall", wallTube, mylar);
    auto* wallPhys = new GeoPhysVol(wallLog);

    auto* gasTube = new GeoTube(0.0, gm(rGas), gm(halfLength));
    auto* gasLog = new GeoLogVol("/SHiP/trackers/straw_gas", gasTube, gas);
    auto* gasPhys = new GeoPhysVol(gasLog);

    wallPhys->add(new GeoNameTag("/SHiP/trackers/straw_gas"));
    wallPhys->add(new GeoIdentifierTag(0));
    wallPhys->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
    wallPhys->add(gasPhys);

    m_strawPhys = wallPhys;
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

    auto* box = new GeoBox(gm(s_halfX), gm(s_halfY), gm(s_trackerMagnetHalfZ));
    auto* log = new GeoLogVol("/SHiP/trackers/tracker_magnet", box, air);
    auto* phys = new GeoPhysVol(log);

    return phys;
}

}  // namespace SHiPGeometry
