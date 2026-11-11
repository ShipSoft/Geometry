// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoIntrusivePtr.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <set>
#include <string>

using SHiP::geometry::Materials;

// The container holds the veto, target, MGC and HCAL children directly. Counts:
//   veto    3 planes × 7 bars                              =   21
//   target  120 layers × (W + Si-X + Si-Y) + 3 Timing_Si   =  363
//   MGC     10 layers × (W + Si-Station)                   =   20
//   HCAL    Σ_section 14 × (Fe + FibreX + FibreY + tiles)
//             S0 14×(3+12²)=2058, S1 14×(3+12²)=2058, S2 14×(3+12²)=2058 = 6174
//                                                            ------
//                                                             6578
TEST_CASE("NeutrinoDetectorChildCount", "[neutrinodetector]") {
    Materials materials;
    SHiP::geometry::NeutrinoDetectorFactory factory(materials);
    GeoPhysVol* snd = factory.build();
    REQUIRE(snd != nullptr);
    CHECK(snd->getNChildVols() == 6578u);  // NOLINT(readability/check)
}

// Every fibre plane of one orientation in a section is the same filled
// envelope, so Geant4 holds each section's fibres once per orientation.
// The fibres inside are named after the section, not the layer, and keep
// the continuous copy numbering across the two sub-layers.
TEST_CASE("FibrePlanesSharedWithinSection", "[neutrinodetector]") {
    Materials materials;
    SHiP::geometry::NeutrinoDetectorFactory factory(materials);
    const GeoIntrusivePtr<GeoPhysVol> snd{factory.build()};
    REQUIRE(snd);

    std::map<std::string, std::set<const GeoVPhysVol*>> planesBySection;
    unsigned placements = 0;
    for (unsigned c = 0; c < snd->getNChildVols(); ++c) {
        const PVConstLink child = snd->getChildVol(c);
        const std::string& logName = child->getLogVol()->getName();
        if (logName.ends_with("/Fibre_ENV")) {
            planesBySection[logName].insert(child.get());
            ++placements;
        }
    }
    CHECK(placements == 3u * 14u * 2u);  // sections × layers × orientations
    REQUIRE(planesBySection.size() == 3);
    for (const auto& [logName, planes] : planesBySection) {
        INFO(logName);
        CHECK(planes.size() == 2);  // one X plane, one Y plane
        const std::string section =
            logName.substr(0, logName.size() - std::string("/Fibre_ENV").size());
        for (const GeoVPhysVol* plane : planes) {
            const unsigned n = plane->getNChildVols();
            REQUIRE(n > 0);
            const std::string name = plane->getNameOfChildVol(0);
            CHECK((name == section + "/FibreX" || name == section + "/FibreY"));
            CHECK(plane->getNameOfChildVol(n - 1) == name);
            // Two sub-layers of n / 2 fibres, numbered continuously.
            CHECK(plane->getIdOfChildVol(0) == 0);
            CHECK(plane->getIdOfChildVol(n / 2) == static_cast<int>(n / 2));
            CHECK(plane->getIdOfChildVol(n - 1) == static_cast<int>(n - 1));
        }
    }
}
