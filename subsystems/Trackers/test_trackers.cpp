// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/Publishing.h"
#include "SHiP/geometry/Trackers/TrackersFactory.h"
#include "SHiP/geometry/Units.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <optional>
#include <string>

using SHiP::geometry::Materials;
using SHiP::geometry::TrackersFactory;
using SHiP::geometry::units::gm;

static const GeoVPhysVol* findChild(const GeoVPhysVol* parent, const std::string& name) {
    for (unsigned int i = 0; i < parent->getNChildVols(); ++i) {
        const PVConstLink child = parent->getChildVol(i);
        if (child->getLogVol()->getName() == name) {
            return &*child;
        }
    }
    return nullptr;
}

static std::optional<unsigned int> findChildIndex(const GeoVPhysVol* parent,
                                                  const std::string& name) {
    for (unsigned int i = 0; i < parent->getNChildVols(); ++i) {
        if (parent->getChildVol(i)->getLogVol()->getName() == name) {
            return i;
        }
    }
    return std::nullopt;
}

// Station sizes and positions are checked against the EDMS envelopes at
// compile time (Layout.h); this checks the factory builds what those
// constants describe.
TEST_CASE("TrackersStationsMatchConstants", "[trackers]") {
    Materials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const std::array stationZ = {TrackersFactory::s_station1Z, TrackersFactory::s_station2Z,
                                 TrackersFactory::s_station3Z, TrackersFactory::s_station4Z};
    for (std::size_t s = 0; s < stationZ.size(); ++s) {
        const std::string name = "/SHiP/trackers/station_" + std::to_string(s + 1);
        INFO("station " << name);
        const auto index = findChildIndex(tc, name);
        REQUIRE(index.has_value());
        const GeoVPhysVol* station = &*tc->getChildVol(*index);
        const auto* box = dynamic_cast<const GeoBox*>(station->getLogVol()->getShape());
        REQUIRE(box != nullptr);
        CHECK(box->getXHalfLength() == gm(TrackersFactory::s_halfX));
        CHECK(box->getYHalfLength() == gm(TrackersFactory::s_halfY));
        CHECK(box->getZHalfLength() == gm(TrackersFactory::s_halfZ));
        CHECK(tc->getXToChildVol(*index).translation().z() ==
              gm(stationZ[s] - TrackersFactory::s_containerCentreZ));
    }
}

// The container holds all 4 stations.
TEST_CASE("TrackersHasFourStations", "[trackers]") {
    Materials materials;
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
    Materials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const GeoVPhysVol* st1 = findChild(tc, "/SHiP/trackers/station_1");
    REQUIRE(st1 != nullptr);
    CHECK(st1->getNChildVols() == static_cast<unsigned>(TrackersFactory::s_nViews));
}

// A view contains a frame plus two straw sub-layers.
TEST_CASE("TrackersViewHasFrameAndSubLayers", "[trackers]") {
    Materials materials;
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

// The inert TrackerMagnet marker is built where s_trackerMagnetZ puts it;
// Layout.h checks at compile time that this is in the gap between station 2
// and the spectrometer-magnet yoke.
TEST_CASE("TrackersHasTrackerMagnet", "[trackers]") {
    Materials materials;
    TrackersFactory factory(materials);
    GeoPhysVol* tc = factory.build();
    REQUIRE(tc != nullptr);
    const auto index = findChildIndex(tc, "/SHiP/trackers/tracker_magnet");
    INFO("tracker_magnet not found");
    REQUIRE(index.has_value());
    const GeoVPhysVol* tm = &*tc->getChildVol(*index);
    const auto* box = dynamic_cast<const GeoBox*>(tm->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getZHalfLength() == gm(TrackersFactory::s_trackerMagnetHalfZ));
    CHECK(tc->getXToChildVol(*index).translation().z() ==
          gm(TrackersFactory::s_trackerMagnetZ - TrackersFactory::s_containerCentreZ));
}

// Inside a PublishingScope, the factory publishes its four stations, keyed by
// their names, under the "Trackers" system; outside one, it publishes nothing.
TEST_CASE("TrackersPublishesStations", "[trackers]") {
    Materials materials;
    SHiP::geometry::Publishers publishers;
    const GeoVPhysVol* tc = nullptr;
    {
        const SHiP::geometry::PublishingScope scope{publishers};
        tc = TrackersFactory(materials).build();
    }
    REQUIRE(tc != nullptr);
    const auto all = publishers.all();
    REQUIRE(all.size() == 1);
    CHECK(all.front()->getName() == "Trackers");
    const auto published = all.front()->getPublishedFPV();
    REQUIRE(published.size() == TrackersFactory::s_nStations);
    for (int i = 1; i <= TrackersFactory::s_nStations; ++i) {
        const std::string name = "/SHiP/trackers/station_" + std::to_string(i);
        INFO("station " << name);
        const auto it = published.find(name);
        REQUIRE(it != published.end());
        CHECK(it->second == findChild(tc, name));
    }

    // Out of scope, a second build leaves the publisher alone (a publish
    // into it would also throw on the reused keys).
    (void)TrackersFactory(materials).build();
    CHECK(all.front()->getPublishedFPV().size() == TrackersFactory::s_nStations);
}
