// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

#include <array>
#include <string>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Trackers (straw tube tracking stations) geometry.
 *
 * Builds 4 straw tracking stations for the SHiP spectrometer. Station
 * envelopes and z-positions follow the GDML reference / subsystem_envelopes.csv:
 * - Station 1: Z 83.57-84.57 m → centre 84.07 m
 * - Station 2: Z 85.57-86.57 m → centre 86.07 m
 * - Station 3: Z 92.57-93.57 m → centre 93.07 m
 * - Station 4: Z 94.57-95.57 m → centre 95.07 m
 * Station envelope (GDML statbox): half 3000 × 3430 × 500 mm.
 *
 * Each station envelope is filled with 4 stereo views. A view is a material
 * frame (FairShip-style hollow rectangle) enclosing a staggered double
 * sub-layer of straw tubes:
 * - Straw: 20 mm outer diameter, 4 m long, horizontal (along X).
 * - Wall:  30 µm Mylar; fill: Ar/CO₂ 70/30 by mass.
 * - View stereo angles about the beam axis Z: +2.3°, -2.3°, +2.3°, -2.3°.
 *
 * The spectrometer dipole between stations 2 and 3 is a separate subsystem
 * (see subsystems/Magnet) and is intentionally not described here.
 */
class TrackersFactory {
   public:
    explicit TrackersFactory(SHiPMaterials& materials);
    ~TrackersFactory() = default;

    /**
     * @brief Build the Trackers geometry.
     * @return Pointer to the container volume holding all 4 stations.
     */
    [[nodiscard]] GeoPhysVol* build();

    // ── Straw / view geometry constants ─────────────────────────────────

    static constexpr int s_nStations = 4;
    static constexpr int s_nViews = 4;      ///< stereo views per station
    static constexpr int s_nSubLayers = 2;  ///< staggered straw layers per view

    static constexpr auto s_strawRadius = 10.0 * units::mm;     ///< 1 cm radius (2 cm diam)
    static constexpr auto s_strawLength = 4000.0 * units::mm;   ///< 4 m, along X
    static constexpr auto s_wallThickness = 0.030 * units::mm;  ///< 30 µm Mylar wall

    /// Active aperture inside a view frame (X = straw length region, Y = pitch).
    static constexpr auto s_apertureX = 4000.0 * units::mm;
    static constexpr auto s_apertureY = 6000.0 * units::mm;

    /// Straws per sub-layer (aperture height / straw diameter).
    static constexpr int s_nStraws =
        static_cast<int>(units::ratio(s_apertureY / (2.0 * s_strawRadius)));

    static constexpr auto s_stereoAngleDeg = 2.3 * units::deg;  ///< |stereo angle| per view

    // View frame (FairShip-style hollow rectangle).
    static constexpr auto s_frameBarX = 100.0 * units::mm;  ///< frame bar width in X
    static constexpr auto s_frameBarY = 100.0 * units::mm;  ///< frame bar width in Y
    static constexpr auto s_frameHalfZ = 22.0 * units::mm;  ///< frame half-thickness in Z

    // ── Tracker magnet ──────────────────────────────────────────────────
    // An inert, air-filled marker volume named "TrackerMagnet", placed in
    // the clear gap between station 2 and the spectrometer-magnet yoke.
    //
    // NOTE: this is NOT a physically-scaled dipole. The spectrometer dipole
    // proper is the separate Magnet subsystem (iron yoke + coils) occupying
    // z = 87.07-92.07 m. The only free space inside the trackers container
    // and clear of that yoke is a ~0.5 m gap, so this marker is sized to fit
    // there. It exists so the tracker magnet has a named placeholder in the
    // geometry; simulation/field code can locate it by the name
    // "/SHiP/trackers/tracker_magnet".
    static constexpr auto s_trackerMagnetZ = 86820.0 * units::mm;    ///< centre
    static constexpr auto s_trackerMagnetHalfZ = 230.0 * units::mm;  ///< half-depth

   private:
    SHiPMaterials& m_materials;

    /// Frame material name in the central SHiPMaterials catalogue.
    std::string m_frameMaterialName = "Aluminium";

    // ── Station envelope from GDML statbox ──────────────────────────────
    static constexpr auto s_halfX = 3000.0 * units::mm;  // 300 cm
    static constexpr auto s_halfY = 3430.0 * units::mm;  // 343 cm (GDML y = 686 cm)
    static constexpr auto s_halfZ = 500.0 * units::mm;   // 50 cm

    // Station Z positions (centres, from origin).
    static constexpr auto s_station1Z = 84070.0 * units::mm;  // 84.07 m
    static constexpr auto s_station2Z = 86070.0 * units::mm;  // 86.07 m
    static constexpr auto s_station3Z = 93070.0 * units::mm;  // 93.07 m
    static constexpr auto s_station4Z = 95070.0 * units::mm;  // 95.07 m

    // Container dimensions (spans all stations).
    static constexpr auto s_containerHalfZ = (s_station4Z - s_station1Z) / 2.0 + s_halfZ;
    static constexpr auto s_containerCentreZ = (s_station1Z + s_station4Z) / 2.0;

    // ── Internal builders ───────────────────────────────────────────────

    /**
     * @brief Build one station envelope and place its 4 stereo views.
     * @param stationIndex 0-based station index (0..3).
     * @return The station envelope physical volume.
     */
    GeoPhysVol* buildStation(int stationIndex);

    /**
     * @brief Build one stereo view: a frame plus two staggered sub-layers.
     * @param stationIndex 0-based parent station index, used for unique names.
     * @param viewIndex    0-based view index within the station (0..3).
     * @return The (unrotated) view physical volume.
     */
    GeoPhysVol* buildView(int stationIndex, int viewIndex);

    /**
     * @brief Build the FairShip-style hollow-rectangle frame for one view.
     * @param stationIndex 0-based parent station index, used for unique names.
     * @param viewIndex    0-based view index within the station.
     */
    GeoPhysVol* buildFrame(int stationIndex, int viewIndex);

    /**
     * @brief Build one sub-layer of s_nStraws parallel straws.
     * @param stationIndex 0-based parent station index, used for unique names.
     * @param viewIndex    0-based parent view index.
     * @param shifted      true for the half-pitch-staggered sub-layer.
     */
    GeoPhysVol* buildSubLayer(int stationIndex, int viewIndex, bool shifted);

    /**
     * @brief Return the shared straw subtree: a Mylar wall cylinder with a gas
     *        core. Built once and reused, since every straw is identical; the
     *        sub-layers place it many times via GeoSerialTransformer.
     */
    GeoPhysVol* buildStraw();

    /// Lazily-built shared straw subtree (see buildStraw()).
    GeoPhysVol* m_strawPhys = nullptr;

    /**
     * @brief Build the inert TrackerMagnet marker volume (air-filled box).
     *
     * Placed in the gap between station 2 and the spectrometer-magnet yoke.
     * See the note on s_trackerMagnetZ for why this is a marker rather than
     * a physically-scaled dipole.
     */
    GeoPhysVol* buildTrackerMagnet();
};

}  // namespace SHiPGeometry
