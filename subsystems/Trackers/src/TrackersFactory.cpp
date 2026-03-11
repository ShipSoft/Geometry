// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Trackers/TrackersFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <string>

namespace SHiPGeometry {

TrackersFactory::TrackersFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TrackersFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");

    // Create container volume that spans all 4 stations
    auto* containerBox = new GeoBox(s_halfX, s_halfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("TrackersContainer", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Create and place individual stations
    const double stationZ[4] = {s_station1Z, s_station2Z, s_station3Z, s_station4Z};

    for (int i = 0; i < 4; ++i) {
        auto* stationBox = new GeoBox(s_halfX, s_halfY, s_halfZ);
        std::string stationName = "TrackerStation_" + std::to_string(i + 1);
        auto* stationLog = new GeoLogVol(stationName, stationBox, air);
        auto* stationPhys = new GeoPhysVol(stationLog);

        // Position relative to container centre
        double relativeZ = stationZ[i] - s_containerCentreZ;
        GeoTrf::Transform3D stationTrf = GeoTrf::Translate3D(0.0, 0.0, relativeZ);

        containerPhys->add(new GeoNameTag(stationName));
        containerPhys->add(new GeoTransform(stationTrf));
        containerPhys->add(stationPhys);
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
