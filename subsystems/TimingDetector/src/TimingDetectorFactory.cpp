// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "TimingDetector/TimingDetectorFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <array>
#include <string>

namespace SHiPGeometry {

using units::gm;

// ── Compile-time validation ──────────────────────────────────────────────
// Kept here rather than in the header so that only this file pays for the
// loops over all bars.
namespace {
using TD = TimingDetectorFactory;

/// Every bar's box, indexed [column][row].
constexpr auto kBars = [] {
    std::array<std::array<checks::Box, TD::s_nRows>, TD::s_nColumns> bars{};
    for (int ic = 0; ic < TD::s_nColumns; ++ic) {
        for (int ir = 0; ir < TD::s_nRows; ++ir) {
            bars[ic][ir] = TD::barBox(ic, ir);
        }
    }
    return bars;
}();

// Bars three or more rows apart are separated in y, so only nearer rows need
// the pairwise check, which keeps it within Clang's constexpr step limit.
// Neighbouring rows and columns do overlap transversely and are kept apart
// only by the z stagger.
constexpr int kRowReach = 3;
}  // namespace

static_assert(
    [] {
        const auto container =
            checks::box(TD::s_containerHalfX, TD::s_containerHalfY, TD::s_containerHalfZ);
        for (const auto& column : kBars) {
            for (const auto& bar : column) {
                if (!checks::contains(container, bar)) {
                    return false;
                }
            }
        }
        return true;
    }(),
    "a timing-detector bar sticks out of the container");
static_assert(kRowReach * TD::s_rowStepY >= 2.0 * TD::s_barHalfY,
              "bars kRowReach rows apart overlap in y: raise kRowReach");
static_assert(
    [] {
        for (int ic = 0; ic < TD::s_nColumns; ++ic) {
            for (int ir = 0; ir < TD::s_nRows; ++ir) {
                for (int jc = 0; jc < TD::s_nColumns; ++jc) {
                    for (int jr = ir; jr < ir + kRowReach && jr < TD::s_nRows; ++jr) {
                        const bool same = (jc == ic && jr == ir);
                        if (!same && checks::overlaps(kBars[ic][ir], kBars[jc][jr])) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }(),
    "two timing-detector bars overlap");

TimingDetectorFactory::TimingDetectorFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TimingDetectorFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* scint = m_materials.requireMaterial("TimDetScint");

    auto const* containerBox =
        new GeoBox(gm(s_containerHalfX), gm(s_containerHalfY), gm(s_containerHalfZ));
    auto* containerLog = new GeoLogVol("/SHiP/timing_detector", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // One reusable bar logvol, shared across all placements (the GeoModel idiom
    // used by the calorimeter bar layers and the upstream-tagger tiles).
    auto* barLog = new GeoLogVol("/SHiP/timing_detector/bar",
                                 new GeoBox(gm(s_barHalfX), gm(s_barHalfY), gm(s_barHalfZ)), scint);

    // 3 columns × 110 rows = 330 bars at the analytic positions of
    // barCentre(), which the header checks at compile time.
    m_barCount = 0;
    for (int ic = 0; ic < s_nColumns; ++ic) {
        for (int ir = 0; ir < s_nRows; ++ir) {
            const auto [x, y, z] = barCentre(ic, ir);
            const std::string name =
                "/SHiP/timing_detector/bar_" + std::to_string(ic) + "_" + std::to_string(ir);
            containerPhys->add(new GeoNameTag(name));
            containerPhys->add(new GeoIdentifierTag(m_barCount));
            containerPhys->add(new GeoTransform(GeoTrf::Translate3D(gm(x), gm(y), gm(z))));
            containerPhys->add(new GeoPhysVol(barLog));
            ++m_barCount;
        }
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
