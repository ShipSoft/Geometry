// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <array>

class GeoLogVol;
class GeoPhysVol;
class GeoVPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Trackers (straw tube tracking stations) geometry.
 *
 * Builds 4 stations of straw tubes spanning the SHiP spectrometer, each station
 * containing four stereo views (layers) of double-staggered straws inside an
 * aluminium support frame.
 *
 * Layout (per station, after stereo rotation about Z):
 *   - 4 layers   : views with stereo angles +/-2.3 deg, alternating
 *     - 1 frame    : hollow rectangle (outer - aperture, GeoShapeSubtraction)
 *     - 2 sub-layers (nominal + half-pitch staggered)
 *       - 300 straws each, 20 mm pitch in Y, straw axis along X (4 m long)
 *         - mylar wall 30 um, Ar/CO2 70/30 gas
 *
 * Station Z positions (centres, mm) match subsystem_envelopes.csv:
 *   station 1: 84070 (z = 83.57-84.57 m)
 *   station 2: 86070
 *   station 3: 93070   (downstream of magnet)
 *   station 4: 95070
 *
 * Container envelope (mm half-sizes) and per-station envelope are unchanged
 * from the previous placeholder, so test_trackers' bound checks still apply.
 */
class TrackersFactory {
   public:
    explicit TrackersFactory(SHiPMaterials& materials);
    ~TrackersFactory() = default;

    /** Build and return the tracker container volume with 4 populated stations. */
    GeoPhysVol* build();

    // ── Per-station envelope (unchanged from the placeholder, GDML statbox) ──
    static constexpr double s_halfX = 3000.0;  // 300 cm
    static constexpr double s_halfY = 3430.0;  // 343 cm
    static constexpr double s_halfZ = 500.0;   //  50 cm

    // Station Z centres (mm from world origin).
    static constexpr double s_station1Z = 84070.0;
    static constexpr double s_station2Z = 86070.0;
    static constexpr double s_station3Z = 93070.0;
    static constexpr double s_station4Z = 95070.0;

    // Container spans station 1 -> station 4 (centre on average of extremes).
    static constexpr double s_containerHalfZ = (s_station4Z - s_station1Z) / 2.0 + s_halfZ;
    static constexpr double s_containerCentreZ = (s_station1Z + s_station4Z) / 2.0;

    // ── Straw geometry (mm, deg) ─────────────────────────────────────────────
    static constexpr int    s_nStations    = 4;
    static constexpr int    s_nLayers      = 4;     // stereo views per station
    static constexpr int    s_nSubLayers   = 2;     // staggered pair per layer
    static constexpr double s_strawRadius  = 10.0;  // 1 cm radius
    static constexpr double s_strawLength  = 4000.0;
    static constexpr double s_wallThick    = 0.030; // 30 um Mylar
    static constexpr double s_apertureX    = 4000.0;
    static constexpr double s_apertureY    = 6000.0;
    static constexpr int    s_nStraws      =
        static_cast<int>(s_apertureY / (2.0 * s_strawRadius));      // 300
    static constexpr double s_stereoAngleDeg = 2.3;
    static constexpr double s_frameWidthX  = 100.0;
    static constexpr double s_frameWidthY  = 100.0;
    static constexpr double s_frameHalfZ   = 22.0;

   private:
    SHiPMaterials& m_materials;

    // Cached LogVols built once per build() and reused across all placements
    // (one wall + one gas LogVol covers all 9600 straws, one shared LogVol per
    // sub-layer variant, etc. — keeps the in-memory tree small).
    GeoLogVol* m_strawWallLog     = nullptr;
    GeoLogVol* m_strawGasLog      = nullptr;
    GeoLogVol* m_subLayerNominal  = nullptr;
    GeoLogVol* m_subLayerShifted  = nullptr;
    GeoLogVol* m_layerLog         = nullptr;
    GeoLogVol* m_frameLog         = nullptr;

    // ── Internal builders ────────────────────────────────────────────────────
    /** Build the LogVols that are shared across stations (straws, sub-layers,
     *  layer envelope, frame). Called once at the top of build(). */
    void buildSharedLogVols();

    /** Build one station volume: 4 stereo-rotated layers in air. */
    GeoPhysVol* buildStation(int stationIndex);

    /** Place one stereo layer (frame + 2 sub-layers) inside @p station. */
    void placeLayer(GeoPhysVol* station, int layerIndex, double signedAngleDeg) const;

    /** Place one sub-layer of @p s_nStraws straws inside @p layer.
     *  @param shifted  if true, the straws are staggered by +1/2 pitch in Y. */
    void placeSubLayer(GeoPhysVol* layer, bool shifted) const;
};

}  // namespace SHiPGeometry
