// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include "Trackers/TrackersFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

using SHiPGeometry::SHiPMaterials;
using SHiPGeometry::TrackersFactory;

static const GeoVPhysVol* findChild(const GeoVPhysVol* parent, const std::string& name) {
    for (unsigned int i = 0; i < parent->getNChildVols(); ++i) {
        PVConstLink child = parent->getChildVol(i);
        if (child->getLogVol()->getName() == name) {
            return &*child;
        }
    }
    return nullptr;
}

// CSV limits: Trackers per-station halfX ≤ 3000, halfY ≤ 3500, halfZ ≤ 500
TEST_CASE("TrackersWithinEnvelope", "[trackers]") {
    SHiPMaterials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* st1 = findChild(tc, "/SHiP/trackers/station_1");
    INFO("TrackerStation_1 not found");
    REQUIRE(st1 != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(st1->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 3000.0);
    CHECK(box->getYHalfLength() <= 3500.0);
    CHECK(box->getZHalfLength() <= 500.0);
}

// The container holds all 4 stations.
TEST_CASE("TrackersHasFourStations", "[trackers]") {
    SHiPMaterials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    for (int i = 1; i <= 4; ++i) {
        const std::string name = "/SHiP/trackers/station_" + std::to_string(i);
        INFO("missing station: " << name);
        CHECK(findChild(tc, name) != nullptr);
    }
}

// Each station is now populated with 4 stereo views (no longer an empty box).
TEST_CASE("TrackersStationHasViews", "[trackers]") {
    SHiPMaterials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* st1 = findChild(tc, "/SHiP/trackers/station_1");
    REQUIRE(st1 != nullptr);
    CHECK(st1->getNChildVols() == static_cast<unsigned>(TrackersFactory::s_nViews));
}

// A view contains a frame plus two straw sub-layers.
TEST_CASE("TrackersViewHasFrameAndSubLayers", "[trackers]") {
    SHiPMaterials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* st1 = findChild(tc, "/SHiP/trackers/station_1");
    REQUIRE(st1 != nullptr);
    const GeoVPhysVol* view0 = findChild(st1, "/SHiP/trackers/station_1/view_0/envelope");
    REQUIRE(view0 != nullptr);
    // 1 frame + 2 sub-layers
    CHECK(view0->getNChildVols() == 3u);  // NOLINT(readability/check)
    const GeoVPhysVol* sub0 = findChild(view0, "/SHiP/trackers/station_1/view_0/sublayer_0_body");
    REQUIRE(sub0 != nullptr);
    // Each sub-layer carries the full straw count.
    CHECK(sub0->getNChildVols() == static_cast<unsigned>(TrackersFactory::s_nStraws));
}

// The inert TrackerMagnet marker is present and fits in the gap before the
// spectrometer-magnet yoke (i.e. it does not overlap station 2 or the yoke).
TEST_CASE("TrackersHasTrackerMagnet", "[trackers]") {
    SHiPMaterials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* tm = findChild(tc, "/SHiP/trackers/tracker_magnet");
    INFO("tracker_magnet not found");
    REQUIRE(tm != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(tm->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Span must stay clear of station 2 (ends 86570 mm) and the Magnet yoke
    // (starts 87070 mm): 86570 <= centre ± halfZ <= 87070.
    const double centre = TrackersFactory::s_trackerMagnetZ;
    const double halfZ = box->getZHalfLength();
    CHECK(centre - halfZ >= 86570.0);
    CHECK(centre + halfZ <= 87070.0);
}
