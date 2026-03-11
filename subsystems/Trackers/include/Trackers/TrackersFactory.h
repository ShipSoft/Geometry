// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Trackers (straw tube tracking stations) geometry
 *
 * Creates 4 tracking stations from GDML reference (statbox solid):
 * - Station 1: Z 83.57-84.57 m → centre 84.07 m
 * - Station 2: Z 85.57-86.57 m → centre 86.07 m
 * - Station 3: Z 92.57-93.57 m → centre 93.07 m
 * - Station 4: Z 94.57-95.57 m → centre 95.07 m
 * GDML statbox: x=600 cm, y=686 cm, z=100 cm → half: 300×343×50 cm
 */
class TrackersFactory {
   public:
    explicit TrackersFactory(SHiPMaterials& materials);
    ~TrackersFactory() = default;

    /**
     * @brief Build the Trackers geometry
     * @return Pointer to container volume with all 4 stations
     */
    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // Station dimensions from GDML statbox (mm)
    static constexpr double s_halfX = 3000.0;  // 300 cm
    static constexpr double s_halfY = 3430.0;  // 343 cm (GDML y=686 cm)
    static constexpr double s_halfZ = 500.0;   // 50 cm

    // Station Z positions (centres, in mm from origin)
    static constexpr double s_station1Z = 84070.0;  // 84.07 m
    static constexpr double s_station2Z = 86070.0;  // 86.07 m
    static constexpr double s_station3Z = 93070.0;  // 93.07 m
    static constexpr double s_station4Z = 95070.0;  // 95.07 m

    // Container dimensions (spans all stations)
    static constexpr double s_containerHalfZ = (s_station4Z - s_station1Z) / 2.0 + s_halfZ;
    static constexpr double s_containerCentreZ = (s_station1Z + s_station4Z) / 2.0;
};

}  // namespace SHiPGeometry
