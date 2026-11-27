// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/Units.h"

#include <array>
#include <string_view>

/**
 * @brief Reference subsystem envelopes, for compile-time placement checks.
 *
 * Hand transcription of subsystem_envelopes.csv, which in turn is taken from
 * "SHiP preliminary infrastructure requirements, layout and integration at
 * ECN3", EDMS 3309666 rev. 0.3, tables 3 and 4. Update both files together.
 *
 * The z ranges of the upstream background tagger, the decay volume, the
 * tracker stations, the spectrometer magnet and the timing detector follow
 * the integration layout drawing SPSXXSHIP0002 version 2026-1.2 (EDMS 3287817
 * v1.2) instead; their transverse sizes are still those of rev. 0.3. The
 * drawing gives the 50 m decay vessel centred at 58.27 m; the decay-volume
 * envelope keeps the 0.2 m margin at each end that rev. 0.3 has.
 *
 * z is measured from the target front face (the SHiP global origin). Only
 * the configurations this geometry builds (WARM muon shield, COMMON) are
 * transcribed; the HYBRID rows are omitted. Empty CSV cells (the hadron
 * stopper's transverse size) are 0 here and must not be used.
 */
namespace SHiP::geometry::envelopes {

using units::LengthMm;

struct Envelope {
    std::string_view name;
    LengthMm zStart;
    LengthMm zEnd;
    LengthMm halfWidthUp;
    LengthMm halfWidthDown;
    LengthMm halfHeightUp;
    LengthMm halfHeightDown;

    [[nodiscard]] constexpr checks::Span z() const { return {.lo = zStart, .hi = zEnd}; }
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
inline constexpr Envelope kHadronStopper{.name="Magnetised hadron stopper", .zStart=2.14 * detail::m, .zEnd=4.44 * detail::m,
    .halfWidthUp=0.0 * detail::m, .halfWidthDown=0.0 * detail::m, .halfHeightUp=0.0 * detail::m, .halfHeightDown=0.0 * detail::m};
inline constexpr std::array<Envelope, 5> kMuonShield{{
    {.name="Muon shield 1 (NC)",  .zStart=4.74 * detail::m, .zEnd=12.14 * detail::m, .halfWidthUp=2.10 * detail::m, .halfWidthDown=2.00 * detail::m, .halfHeightUp=1.60 * detail::m, .halfHeightDown=1.60 * detail::m},
    {.name="Muon shield 2 (NC)", .zStart=12.24 * detail::m, .zEnd=19.24 * detail::m, .halfWidthUp=0.35 * detail::m, .halfWidthDown=1.15 * detail::m, .halfHeightUp=0.35 * detail::m, .halfHeightDown=1.55 * detail::m},
    {.name="Muon shield 3 (NC)", .zStart=19.34 * detail::m, .zEnd=24.34 * detail::m, .halfWidthUp=0.10 * detail::m, .halfWidthDown=1.65 * detail::m, .halfHeightUp=1.95 * detail::m, .halfHeightDown=2.30 * detail::m},
    {.name="Muon shield 4 (NC)", .zStart=24.44 * detail::m, .zEnd=26.12 * detail::m, .halfWidthUp=0.10 * detail::m, .halfWidthDown=1.65 * detail::m, .halfHeightUp=1.95 * detail::m, .halfHeightDown=2.30 * detail::m},
    {.name="Muon shield 5 (NC)", .zStart=26.22 * detail::m, .zEnd=32.22 * detail::m, .halfWidthUp=0.50 * detail::m, .halfWidthDown=1.60 * detail::m, .halfHeightUp=0.10 * detail::m, .halfHeightDown=2.15 * detail::m},
}};
inline constexpr Envelope kNeutrinoDetector{.name="Scattering and neutrino detector", .zStart=26.40 * detail::m, .zEnd=31.50 * detail::m,
    .halfWidthUp=0.30 * detail::m, .halfWidthDown=0.40 * detail::m, .halfHeightUp=0.30 * detail::m, .halfHeightDown=0.40 * detail::m};
inline constexpr Envelope kUpstreamTagger{.name="Upstream background tagger", .zStart=32.67 * detail::m, .zEnd=33.07 * detail::m,
    .halfWidthUp=0.75 * detail::m, .halfWidthDown=0.75 * detail::m, .halfHeightUp=1.60 * detail::m, .halfHeightDown=1.60 * detail::m};
inline constexpr Envelope kDecayVolume{.name="Decay volume", .zStart=33.07 * detail::m, .zEnd=83.47 * detail::m,
    .halfWidthUp=0.70 * detail::m, .halfWidthDown=2.20 * detail::m, .halfHeightUp=1.55 * detail::m, .halfHeightDown=3.20 * detail::m};
/// Thickness of the SBT shell around the decay volume (the CSV lists it as a
/// half-width/height of 0.46 m).
inline constexpr auto kSurroundTaggerThickness = 0.46 * detail::m;
inline constexpr std::array<Envelope, 4> kTrackerStations{{
    {.name="Straw Tracker station 1", .zStart=83.72 * detail::m, .zEnd=84.72 * detail::m, .halfWidthUp=3.00 * detail::m, .halfWidthDown=3.00 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
    {.name="Straw Tracker station 2", .zStart=85.72 * detail::m, .zEnd=86.72 * detail::m, .halfWidthUp=3.00 * detail::m, .halfWidthDown=3.00 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
    {.name="Straw Tracker station 3", .zStart=92.72 * detail::m, .zEnd=93.72 * detail::m, .halfWidthUp=3.00 * detail::m, .halfWidthDown=3.00 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
    {.name="Straw Tracker station 4", .zStart=94.72 * detail::m, .zEnd=95.72 * detail::m, .halfWidthUp=3.00 * detail::m, .halfWidthDown=3.00 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
}};
inline constexpr Envelope kMagnet{.name="Main spectrometer magnet", .zStart=87.22 * detail::m, .zEnd=92.22 * detail::m,
    .halfWidthUp=3.25 * detail::m, .halfWidthDown=3.25 * detail::m, .halfHeightUp=4.30 * detail::m, .halfHeightDown=4.30 * detail::m};
inline constexpr Envelope kTimingDetector{.name="Timing detector", .zStart=96.14 * detail::m, .zEnd=96.60 * detail::m,
    .halfWidthUp=2.75 * detail::m, .halfWidthDown=2.75 * detail::m, .halfHeightUp=3.25 * detail::m, .halfHeightDown=3.25 * detail::m};
inline constexpr std::array<Envelope, 3> kCalorimeter{{
    {.name="Split ECAL (front)", .zStart=96.87 * detail::m, .zEnd=97.07 * detail::m, .halfWidthUp=2.25 * detail::m, .halfWidthDown=2.25 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
    {.name="Split ECAL (back)",  .zStart=98.07 * detail::m, .zEnd=98.67 * detail::m, .halfWidthUp=2.75 * detail::m, .halfWidthDown=2.75 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
    {.name="HCAL",               .zStart=98.77 * detail::m, .zEnd=99.77 * detail::m, .halfWidthUp=3.00 * detail::m, .halfWidthDown=3.00 * detail::m, .halfHeightUp=3.50 * detail::m, .halfHeightDown=3.50 * detail::m},
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

}  // namespace SHiP::geometry::envelopes
