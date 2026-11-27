// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/UpstreamTagger/UpstreamTaggerFactory.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoIntrusivePtr.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoVFullPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>
#include <GeoModelKernel/GeoVolumeCursor.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

using SHiP::geometry::Materials;

// UpstreamTagger container must be a GeoVFullPhysVol (sensitive tree-top)
TEST_CASE("UBTHasSensitiveVolume", "[upstreamtagger]") {
    Materials materials;
    SHiP::geometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    CHECK(dynamic_cast<const GeoVFullPhysVol*>(ubt) != nullptr);
}

// The tile plane is built from seven abutting tile regions.
TEST_CASE("UBTHasSevenRegions", "[upstreamtagger]") {
    Materials materials;
    SHiP::geometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);
    CHECK(ubt->getNChildVols() == 7u);  // NOLINT(readability/check)
}

// Every region is fully tiled; the totals are fixed by the region geometry:
//   fine blocks  2 × (20×20)      =   800
//   coarse central  30×10          =   300
//   coarse bands 2 × (50×75)       =  7500
//   extensions   2 × (60×320)      = 38400
//                                   ------
//                                    47000 tiles
TEST_CASE("UBTTileCount", "[upstreamtagger]") {
    Materials materials;
    SHiP::geometry::UpstreamTaggerFactory factory(materials);
    GeoVPhysVol* ubt = factory.build();
    REQUIRE(ubt != nullptr);

    unsigned int totalTiles = 0;
    for (unsigned int i = 0; i < ubt->getNChildVols(); ++i) {
        totalTiles += ubt->getChildVol(i)->getNChildVols();
    }
    CHECK(totalTiles == 47000u);  // NOLINT(readability/check)
}

// Every tile sits on its region's grid, with copy number ix·nY + iy. This
// pins the positions and numbering of the original per-tile placements, so
// that how the grid is built (one placement per tile or one serial
// transformer per region) cannot change them.
TEST_CASE("UBTTileGridAndCopyNumbers", "[upstreamtagger]") {
    Materials materials;
    SHiP::geometry::UpstreamTaggerFactory factory(materials);
    const GeoIntrusivePtr<GeoVPhysVol> ubt{factory.build()};
    REQUIRE(ubt);

    constexpr double kTol = 1e-9;  // mm
    for (unsigned int r = 0; r < ubt->getNChildVols(); ++r) {
        const PVConstLink env = ubt->getChildVol(r);
        const auto* envBox = dynamic_cast<const GeoBox*>(env->getLogVol()->getShape());
        REQUIRE(envBox != nullptr);
        REQUIRE(env->getNChildVols() > 0);
        const auto* tileBox =
            dynamic_cast<const GeoBox*>(env->getChildVol(0)->getLogVol()->getShape());
        REQUIRE(tileBox != nullptr);

        const double pitch = 2.0 * tileBox->getXHalfLength();
        const auto nX = static_cast<unsigned>(std::lround(2.0 * envBox->getXHalfLength() / pitch));
        const auto nY = static_cast<unsigned>(std::lround(2.0 * envBox->getYHalfLength() / pitch));
        REQUIRE(env->getNChildVols() == nX * nY);

        const double x0 = -0.5 * (nX - 1) * pitch;
        const double y0 = -0.5 * (nY - 1) * pitch;
        // A cursor walks the children once; getXToChildVol(k) would rescan
        // them for every k.
        unsigned int k = 0;
        for (GeoVolumeCursor tile(env); !tile.atEnd(); tile.next(), ++k) {
            const unsigned ix = k / nY;
            const unsigned iy = k % nY;
            const GeoTrf::Vector3D t = tile.getTransform().translation();
            if (std::abs(t.x() - (x0 + (ix * pitch))) > kTol ||
                std::abs(t.y() - (y0 + (iy * pitch))) > kTol || std::abs(t.z()) > kTol ||
                tile.getId() != static_cast<int>(k)) {
                FAIL(env->getLogVol()->getName()
                     << " tile " << k << " at (" << t.x() << ", " << t.y() << ", " << t.z()
                     << "), id " << tile.getId().value_or(-1));
            }
        }
        CHECK(k == nX * nY);
    }
}
