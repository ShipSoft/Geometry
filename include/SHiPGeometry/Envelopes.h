// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/StaticChecks.h"
#include "SHiPGeometry/Units.h"

#include <array>
#include <string_view>

/**
 * @brief Reference subsystem envelopes, for compile-time placement checks.
 *
 * Hand transcription of subsystem_envelopes.csv, which in turn is taken from
 * "SHiP preliminary infrastructure requirements, layout and integration at
 * ECN3", EDMS 3309666 rev. 0.3, tables 3 and 4. Update both files together.
 *
 * z is measured from the target front face (the SHiP global origin). Only
 * the configurations this geometry builds (WARM muon shield, COMMON) are
 * transcribed; the HYBRID rows are omitted. Empty CSV cells (the hadron
 * stopper's transverse size) are 0 here and must not be used.
 */
namespace SHiPGeometry::Envelopes {

using units::LengthMm;

struct Envelope {
    std::string_view name;
    LengthMm zStart;
    LengthMm zEnd;
    LengthMm halfWidthUp;
    LengthMm halfWidthDown;
    LengthMm halfHeightUp;
    LengthMm halfHeightDown;

    [[nodiscard]] constexpr checks::Span z() const { return {zStart, zEnd}; }
    [[nodiscard]] constexpr LengthMm maxHalfWidth() const {
        return halfWidthUp < halfWidthDown ? halfWidthDown : halfWidthUp;
    }
    [[nodiscard]] constexpr LengthMm maxHalfHeight() const {
        return halfHeightUp < halfHeightDown ? halfHeightDown : halfHeightUp;
    }
};

namespace detail {
inline constexpr auto m = units::m;
}  // namespace detail

// clang-format off
inline constexpr Envelope kHadronStopper{"Magnetised hadron stopper", 2.14 * detail::m, 4.44 * detail::m,
    0.0 * detail::m, 0.0 * detail::m, 0.0 * detail::m, 0.0 * detail::m};
inline constexpr std::array<Envelope, 5> kMuonShield{{
    {"Muon shield 1 (NC)",  4.74 * detail::m, 12.14 * detail::m, 2.10 * detail::m, 2.00 * detail::m, 1.60 * detail::m, 1.60 * detail::m},
    {"Muon shield 2 (NC)", 12.24 * detail::m, 19.24 * detail::m, 0.35 * detail::m, 1.15 * detail::m, 0.35 * detail::m, 1.55 * detail::m},
    {"Muon shield 3 (NC)", 19.34 * detail::m, 24.34 * detail::m, 0.10 * detail::m, 1.65 * detail::m, 1.95 * detail::m, 2.30 * detail::m},
    {"Muon shield 4 (NC)", 24.44 * detail::m, 26.12 * detail::m, 0.10 * detail::m, 1.65 * detail::m, 1.95 * detail::m, 2.30 * detail::m},
    {"Muon shield 5 (NC)", 26.22 * detail::m, 32.22 * detail::m, 0.50 * detail::m, 1.60 * detail::m, 0.10 * detail::m, 2.15 * detail::m},
}};
inline constexpr Envelope kNeutrinoDetector{"Scattering and neutrino detector", 26.40 * detail::m, 31.50 * detail::m,
    0.30 * detail::m, 0.40 * detail::m, 0.30 * detail::m, 0.40 * detail::m};
inline constexpr Envelope kUpstreamTagger{"Upstream background tagger", 32.52 * detail::m, 32.92 * detail::m,
    0.75 * detail::m, 0.75 * detail::m, 1.60 * detail::m, 1.60 * detail::m};
inline constexpr Envelope kDecayVolume{"Decay volume", 32.92 * detail::m, 83.32 * detail::m,
    0.70 * detail::m, 2.20 * detail::m, 1.55 * detail::m, 3.20 * detail::m};
/// Thickness of the SBT shell around the decay volume (the CSV lists it as a
/// half-width/height of 0.46 m).
inline constexpr auto kSurroundTaggerThickness = 0.46 * detail::m;
inline constexpr std::array<Envelope, 4> kTrackerStations{{
    {"Straw Tracker station 1", 83.57 * detail::m, 84.57 * detail::m, 3.00 * detail::m, 3.00 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
    {"Straw Tracker station 2", 85.57 * detail::m, 86.57 * detail::m, 3.00 * detail::m, 3.00 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
    {"Straw Tracker station 3", 92.57 * detail::m, 93.57 * detail::m, 3.00 * detail::m, 3.00 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
    {"Straw Tracker station 4", 94.57 * detail::m, 95.57 * detail::m, 3.00 * detail::m, 3.00 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
}};
inline constexpr Envelope kMagnet{"Main spectrometer magnet", 87.07 * detail::m, 92.07 * detail::m,
    3.25 * detail::m, 3.25 * detail::m, 4.30 * detail::m, 4.30 * detail::m};
inline constexpr Envelope kTimingDetector{"Timing detector", 95.82 * detail::m, 96.32 * detail::m,
    2.75 * detail::m, 2.75 * detail::m, 3.25 * detail::m, 3.25 * detail::m};
inline constexpr std::array<Envelope, 3> kCalorimeter{{
    {"Split ECAL (front)", 96.87 * detail::m, 97.07 * detail::m, 2.25 * detail::m, 2.25 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
    {"Split ECAL (back)",  98.07 * detail::m, 98.67 * detail::m, 2.75 * detail::m, 2.75 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
    {"HCAL",               98.77 * detail::m, 99.77 * detail::m, 3.00 * detail::m, 3.00 * detail::m, 3.50 * detail::m, 3.50 * detail::m},
}};
// clang-format on

/// Union of several consecutive envelopes, from the first start to the last
/// end, with the largest transverse size of any of them.
template <std::size_t N>
[[nodiscard]] constexpr Envelope hull(std::string_view name, const std::array<Envelope, N>& parts) {
    Envelope out{name,
                 parts.front().zStart,
                 parts.back().zEnd,
                 parts.front().maxHalfWidth(),
                 parts.front().maxHalfWidth(),
                 parts.front().maxHalfHeight(),
                 parts.front().maxHalfHeight()};
    for (const auto& part : parts) {
        out.zStart = part.zStart < out.zStart ? part.zStart : out.zStart;
        out.zEnd = out.zEnd < part.zEnd ? part.zEnd : out.zEnd;
        const auto w = part.maxHalfWidth();
        const auto h = part.maxHalfHeight();
        out.halfWidthUp = out.halfWidthDown = out.halfWidthUp < w ? w : out.halfWidthUp;
        out.halfHeightUp = out.halfHeightDown = out.halfHeightUp < h ? h : out.halfHeightUp;
    }
    return out;
}

}  // namespace SHiPGeometry::Envelopes
