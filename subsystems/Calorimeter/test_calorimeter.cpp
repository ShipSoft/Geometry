// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Calorimeter/CaloFibreHPLayer.h"
#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoIntrusivePtr.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoVolumeCursor.h>

#include <algorithm>
#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

using SHiP::geometry::CalorimeterFactory;
using SHiP::geometry::Materials;

TEST_CASE("CalorimeterBuilds", "[calorimeter]") {
    Materials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    const auto* box = dynamic_cast<const GeoBox*>(calo->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    // Fixed envelope: 3.00 × 3.50 × 1.45 m half-sizes
    CHECK(box->getXHalfLength() == 3000.0);
    CHECK(box->getYHalfLength() == 3500.0);
    CHECK(box->getZHalfLength() == 1450.0);
}

TEST_CASE("CalorimeterHasChildren", "[calorimeter]") {
    Materials materials;
    CalorimeterFactory factory(materials);
    GeoPhysVol* calo = factory.build();
    REQUIRE(calo != nullptr);
    // One volume per module per non-air-gap layer, so the sequencer cannot
    // silently drop or duplicate a placement.
    constexpr unsigned kExpectedChildren = SHiP::geometry::calo::kModuleNX *
                                           SHiP::geometry::calo::kModuleNY *
                                           SHiP::geometry::calo::kVolsPerModule;
    CHECK(calo->getNChildVols() == kExpectedChildren);  // NOLINT(readability/check)
}

TEST_CASE("LayerCopyNumbersIdentifyModuleAndLayer", "[calorimeter]") {
    // Shared layers carry no module of their own, so the copy number of each
    // layer placement must identify both the module and the layer.
    namespace calo = SHiP::geometry::calo;
    using SHiP::geometry::units::gm;
    Materials materials;
    CalorimeterFactory factory(materials);
    const GeoIntrusivePtr<GeoPhysVol> calorimeter{factory.build()};
    const double x0 = -0.5 * (calo::kModuleNX - 1) * gm(calo::kModulePitchX);
    const double y0 = -0.5 * (calo::kModuleNY - 1) * gm(calo::kModulePitchY);

    std::set<int> seen;
    for (unsigned c = 0; c < calorimeter->getNChildVols(); ++c) {
        const auto id = calorimeter->getIdOfChildVol(c);
        REQUIRE(id.has_value());
        CHECK(seen.insert(*id).second);
        // Modules are placed one after another, each layer by layer.
        const int module = static_cast<int>(c) / calo::kVolsPerModule;
        const int layer = static_cast<int>(c) % calo::kVolsPerModule;
        const int ix = module % calo::kModuleNX;
        const int iy = module / calo::kModuleNX;
        const GeoTrf::Vector3D t = calorimeter->getXToChildVol(c).translation();
        if (calo::moduleOfLayerCopyNumber(*id) != calo::moduleIndex(ix, iy) ||
            calo::layerOfLayerCopyNumber(*id) != layer ||
            t.x() != x0 + (ix * gm(calo::kModulePitchX)) ||
            t.y() != y0 + (iy * gm(calo::kModulePitchY))) {
            FAIL("child " << c << " (" << calorimeter->getNameOfChildVol(c) << ") has copy number "
                          << *id << " at (" << t.x() << ", " << t.y() << ")");
        }
    }
}

// ── HPL fibre layout ─────────────────────────────────────────────────────────

namespace {

namespace calo = SHiP::geometry::calo;
using SHiP::geometry::units::gm;
using SHiP::geometry::units::mm;

/// Distance between the centres of two fibres, in mm.
double fibreDistance(int s1, int i1, int s2, int i2) {
    const auto a = calo::fibreCentre(s1, i1);
    const auto b = calo::fibreCentre(s2, i2);
    return std::hypot(gm(a.across) - gm(b.across), gm(a.z) - gm(b.z));
}

/// A mother volume and the materials for building one HPL layer into it.
/// The intrusive pointer releases the tree however the test exits.
struct HplBuild {
    Materials materials;
    const GeoMaterial* al = materials.requireMaterial("Aluminium");
    const GeoMaterial* ps = materials.requireMaterial("Polystyrene");
    const GeoMaterial* air = materials.requireMaterial("Air");
    GeoIntrusivePtr<GeoPhysVol> mother{new GeoPhysVol(new GeoLogVol(
        "mother", new GeoBox(gm(calo::kPlateXY), gm(calo::kPlateXY), gm(calo::kPlateXY)), air))};

    void build(SHiP::geometry::units::LengthMm casingXY,
               SHiP::geometry::units::LengthMm casingZ) const {
        // makeLayer() throws before creating anything, so a rejected casing
        // never reaches the mother.
        GeoPhysVol* casing = SHiP::geometry::calo_fibre_hp::makeLayer(
            al, ps, casingXY, casingZ, calo::kFiberDiameter, calo::kFiberCoreDiameter, true);
        mother->add(new GeoNameTag("T_HPL_Casing"));
        mother->add(new GeoTransform(GeoTrf::Transform3D::Identity()));
        mother->add(casing);
    }
};

}  // namespace

TEST_CASE("HplFibreCountsPerSublayer", "[calorimeter]") {
    CHECK(calo::fibresPerSublayer(0) == 1799);
    CHECK(calo::fibresPerSublayer(1) == 1800);
    CHECK(calo::fibresPerSublayer(2) == 1799);
    CHECK(calo::kFibresPerHpl == 5398);
}

TEST_CASE("HplFibresCloseTouchWithoutOverlap", "[calorimeter]") {
    const double diameter = gm(calo::kFiberDiameter);
    // Each outer fibre j sits between middle fibres j and j + 1; neighbours
    // within a row are one pitch (one diameter) apart. Track the extremes so
    // a single wrong centre shows up without thousands of assertions.
    double minDist = std::numeric_limits<double>::max();
    double maxDist = 0.0;
    const auto record = [&](double d) {
        minDist = std::min(minDist, d);
        maxDist = std::max(maxDist, d);
    };
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        for (int i = 0; i + 1 < calo::fibresPerSublayer(s); ++i) {
            record(fibreDistance(s, i, s, i + 1));
        }
    }
    for (const int outer : {0, 2}) {
        for (int j = 0; j < calo::fibresPerSublayer(outer); ++j) {
            record(fibreDistance(outer, j, 1, j));
            record(fibreDistance(outer, j, 1, j + 1));
        }
    }
    CHECK(minDist == Catch::Approx(diameter));
    CHECK(maxDist == Catch::Approx(diameter));
    // Outer rows are two row spacings apart, well clear of each other.
    CHECK(fibreDistance(0, 0, 2, 0) > diameter);
}

TEST_CASE("HplFibresInsideCasing", "[calorimeter]") {
    const double r = 0.5 * gm(calo::kFiberDiameter);
    const double halfXY = 0.5 * gm(calo::kPlateXY);
    const double halfZ = 0.5 * gm(calo::kHplThickness);
    constexpr double kTol = 1e-9;  // the outermost middle fibres are flush with the casing
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        const int n = calo::fibresPerSublayer(s);
        for (const int i : {0, n - 1}) {
            const auto c = calo::fibreCentre(s, i);
            CHECK(std::abs(gm(c.across)) + r <= halfXY + kTol);
            CHECK(std::abs(gm(c.z)) + r <= halfZ);
        }
    }
}

TEST_CASE("HplBuilderPlacesSharedCentres", "[calorimeter]") {
    const HplBuild hpl;
    hpl.build(calo::kPlateXY, calo::kHplThickness);
    REQUIRE(hpl.mother->getNChildVols() == 1);
    const PVConstLink casing = hpl.mother->getChildVol(0);
    REQUIRE(casing->getNChildVols() == static_cast<unsigned>(calo::kFibresPerHpl));

    // Children are placed sublayer by sublayer, fibre by fibre (fibres along Y,
    // so the across-layer coordinate is X).
    unsigned child = 0;
    for (int s = 0; s < calo::kHplSublayers; ++s) {
        for (int i = 0; i < calo::fibresPerSublayer(s); ++i, ++child) {
            const auto c = calo::fibreCentre(s, i);
            // A concrete Vector3D, not auto: translation() is a view into the
            // temporary transform.
            const GeoTrf::Vector3D t = casing->getXToChildVol(child).translation();
            if (t.x() != gm(c.across) || t.y() != 0.0 || t.z() != gm(c.z)) {
                FAIL("fibre S" << s << "_F" << i << " at (" << t.x() << ", " << t.y() << ", "
                               << t.z() << ")");
            }
            if (casing->getIdOfChildVol(child) != calo::fibreCopyNumber(s, i)) {
                FAIL("fibre S" << s << "_F" << i << " has the wrong copy number");
            }
        }
    }
}

TEST_CASE("HplCasingsSharedAcrossLayers", "[calorimeter]") {
    // Every HPL layer of one orientation places the same casing, so the
    // fibres exist once per orientation rather than once per layer.
    Materials materials;
    CalorimeterFactory factory(materials);
    const GeoIntrusivePtr<GeoPhysVol> calorimeter{factory.build()};
    std::set<const GeoVPhysVol*> casings;
    unsigned placements = 0;
    // The casings sit directly in the container, at the same depth as the
    // bar layers.
    for (unsigned c = 0; c < calorimeter->getNChildVols(); ++c) {
        const PVConstLink child = calorimeter->getChildVol(c);
        if (child->getLogVol()->getName() == "HPL_CasingLog") {
            casings.insert(child.get());
            ++placements;
        }
    }
    const auto isHpl = [](calo::LayerCode code) {
        return code == calo::LayerCode::FibreHPL_X || code == calo::LayerCode::FibreHPL_Y;
    };
    const auto hplPerModule =
        static_cast<unsigned>(std::ranges::count_if(calo::kEcalLayers, isHpl) +
                              std::ranges::count_if(calo::kHcalLayers, isHpl));
    REQUIRE(hplPerModule > 0);
    CHECK(placements == calo::kModuleNX * calo::kModuleNY * hplPerModule);
    CHECK(casings.size() == 2);
}

TEST_CASE("BarLayersSharedAcrossSlots", "[calorimeter]") {
    // Every bar layer of one type places the same filled envelope, whose bars
    // sit at the layer's pitch with copy number = bar index.
    Materials materials;
    CalorimeterFactory factory(materials);
    const GeoIntrusivePtr<GeoPhysVol> calorimeter{factory.build()};

    struct BarType {
        calo::LayerCode code;
        std::string envName;
        double pitch;
        int nBars;
        bool alongX;  // bars replicated along X
    };
    const std::array<BarType, 4> types{{
        {.code = calo::LayerCode::WidePVT_H,
         .envName = "/SHiP/calorimeter/wide_pvt_h_env",
         .pitch = gm(calo::kWidePVTBarPitch),
         .nBars = calo::kWidePVTBarCount,
         .alongX = false},
        {.code = calo::LayerCode::WidePVT_V,
         .envName = "/SHiP/calorimeter/wide_pvt_v_env",
         .pitch = gm(calo::kWidePVTBarPitch),
         .nBars = calo::kWidePVTBarCount,
         .alongX = true},
        {.code = calo::LayerCode::ThinPS_H,
         .envName = "/SHiP/calorimeter/thin_ps_h_env",
         .pitch = gm(calo::kThinPSBarPitch),
         .nBars = calo::kThinPSBarCount,
         .alongX = false},
        {.code = calo::LayerCode::ThinPS_V,
         .envName = "/SHiP/calorimeter/thin_ps_v_env",
         .pitch = gm(calo::kThinPSBarPitch),
         .nBars = calo::kThinPSBarCount,
         .alongX = true},
    }};

    for (const auto& type : types) {
        std::set<const GeoVPhysVol*> layers;
        unsigned placements = 0;
        for (unsigned c = 0; c < calorimeter->getNChildVols(); ++c) {
            const PVConstLink child = calorimeter->getChildVol(c);
            if (child->getLogVol()->getName() == type.envName) {
                layers.insert(child.get());
                ++placements;
            }
        }
        const auto perModule =
            static_cast<unsigned>(std::ranges::count(calo::kEcalLayers, type.code) +
                                  std::ranges::count(calo::kHcalLayers, type.code));
        INFO(type.envName);
        REQUIRE(perModule > 0);
        CHECK(placements == calo::kModuleNX * calo::kModuleNY * perModule);
        REQUIRE(layers.size() == 1);

        const GeoVPhysVol* layer = *layers.begin();
        REQUIRE(layer->getNChildVols() == static_cast<unsigned>(type.nBars));
        const double first = -0.5 * (type.nBars - 1) * type.pitch;
        int i = 0;
        for (GeoVolumeCursor bar(layer); !bar.atEnd(); bar.next(), ++i) {
            const GeoTrf::Vector3D t = bar.getTransform().translation();
            const double across = first + (i * type.pitch);
            const double x = type.alongX ? across : 0.0;
            const double y = type.alongX ? 0.0 : across;
            if (t.x() != x || t.y() != y || t.z() != 0.0 || bar.getId() != i) {
                FAIL(type.envName << " bar " << i << " at (" << t.x() << ", " << t.y() << ", "
                                  << t.z() << "), id " << bar.getId().value_or(-1));
            }
        }
    }
}

TEST_CASE("HplBuilderRejectsThinCasing", "[calorimeter]") {
    const HplBuild hpl;
    // 3 mm < 1.2 mm * (1 + sqrt(3)) ≈ 3.28 mm
    CHECK_THROWS_AS(hpl.build(calo::kPlateXY, 3.0 * mm), std::runtime_error);
    // The check runs before the casing is placed, so nothing is left behind.
    CHECK(hpl.mother->getNChildVols() == 0);
}

TEST_CASE("HplBuilderRejectsNarrowCasing", "[calorimeter]") {
    const HplBuild hpl;
    // 2 mm < 2 * 1.2 mm: the outer sublayers would hold no fibres.
    CHECK_THROWS_AS(hpl.build(2.0 * mm, calo::kHplThickness), std::runtime_error);
    CHECK(hpl.mother->getNChildVols() == 0);
}

TEST_CASE("HplFibreCountToleratesRounding", "[calorimeter]") {
    // 0.3 / 0.1 is 2.9999999999999996 in double precision; plain truncation
    // would drop a fibre from every row.
    CHECK(calo::fibresPerSublayer(1, 0.3 * mm, 0.1 * mm) == 3);
    CHECK(calo::fibresPerSublayer(0, 0.3 * mm, 0.1 * mm) == 2);
}
