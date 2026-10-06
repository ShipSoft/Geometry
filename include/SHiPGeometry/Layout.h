// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "Calorimeter/CalorimeterConstants.h"
#include "Cavern/CavernFactory.h"
#include "DecayVolume/SBTConstants.h"
#include "Magnet/MagnetFactory.h"
#include "MuonShield/MuonShieldFactory.h"
#include "SHiPGeometry/Envelopes.h"
#include "SHiPGeometry/StaticChecks.h"
#include "SHiPGeometry/Units.h"
#include "Target/TargetFactory.h"
#include "TimingDetector/TimingDetectorFactory.h"
#include "Trackers/TrackersFactory.h"
#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"

#include <array>
#include <string_view>

/**
 * @brief Where each subsystem container sits in the world volume.
 *
 * SHiPGeometryBuilder places the subsystems from this table, and the
 * static_asserts below check the layout as a whole: ordering along the beam,
 * no unintended overlaps, everything inside the world, and every container
 * inside its EDMS envelope (Envelopes.h). Container half-lengths are read
 * from the factories, so resizing a container is checked here as well.
 *
 * Coordinates are in the world frame, whose origin is the front face of the
 * first target disk.
 */
namespace SHiPGeometry::Layout {

using units::LengthMm;

struct Slot {
    std::string_view path;
    int id;
    LengthMm x;
    LengthMm y;
    LengthMm z;
    LengthMm halfX;
    LengthMm halfY;
    LengthMm halfZ;

    [[nodiscard]] constexpr checks::Box box() const {
        return checks::box(x, y, z, halfX, halfY, halfZ);
    }
    [[nodiscard]] constexpr checks::Span zSpan() const { return box().z; }
};

namespace detail {
inline constexpr auto mm = units::mm;
inline constexpr auto m = units::m;
inline constexpr auto zero = 0.0 * units::mm;
}  // namespace detail

/// The target frame (z = 0 at the first disk) is the world frame, so the
/// vacuum box sits at minus the target frame's offset inside it.
inline constexpr Slot kTarget{"/SHiP/target",
                              1,
                              detail::zero,
                              -TargetFactory::s_targetAreaPosY,
                              -TargetFactory::s_targetAreaPosZ,
                              TargetFactory::s_vacuumBoxHalfX,
                              TargetFactory::s_vacuumBoxHalfY,
                              TargetFactory::s_vacuumBoxHalfZ};

/// GDML z range 204-3148.66 cm, centre 1676.33 cm.
inline constexpr Slot kMuonShield{"/SHiP/muon_shield",
                                  2,
                                  detail::zero,
                                  detail::zero,
                                  16763.3 * detail::mm,
                                  MuonShieldFactory::s_areaHalfX,
                                  MuonShieldFactory::s_areaHalfY,
                                  MuonShieldFactory::s_areaHalfZ};

/// z 26.40-31.50 m (WARM muon-shield configuration). The SND sits within the
/// downstream end of the muon-shield region, so the two containers overlap
/// by design (see kAllowedOverlaps).
inline constexpr Slot kNeutrinoDetector{"/SHiP/neutrino_detector",
                                        9,
                                        detail::zero,
                                        detail::zero,
                                        28.95 * detail::m,
                                        NeutrinoDetectorFactory::s_halfX,
                                        NeutrinoDetectorFactory::s_halfY,
                                        NeutrinoDetectorFactory::s_halfZ};

inline constexpr Slot kUpstreamTagger{"/SHiP/upstream_tagger",
                                      3,
                                      detail::zero,
                                      detail::zero,
                                      32.72 * detail::m,
                                      UpstreamTaggerFactory::s_halfX,
                                      UpstreamTaggerFactory::s_halfY,
                                      UpstreamTaggerFactory::s_halfZ};

inline constexpr Slot kDecayVolume{"/SHiP/decay_volume", 4,
                                   detail::zero,         detail::zero,
                                   58.12 * detail::m,    SBT::kEnvelopeHalfX,
                                   SBT::kEnvelopeHalfY,  SBT::kEnvelopeHalfZ};

/// The factory positions its stations relative to s_containerCentreZ, so the
/// container goes there and the stations land on their absolute z. The
/// container spans the magnet (stations 1-2 upstream, 3-4 downstream).
inline constexpr Slot kTrackers{"/SHiP/trackers",
                                5,
                                detail::zero,
                                detail::zero,
                                TrackersFactory::s_containerCentreZ,
                                TrackersFactory::s_halfX,
                                TrackersFactory::s_halfY,
                                TrackersFactory::s_containerHalfZ};

inline constexpr Slot kMagnet{"/SHiP/magnet",
                              6,
                              detail::zero,
                              detail::zero,
                              89.57 * detail::m,
                              MagnetFactory::s_containerHalfX,
                              MagnetFactory::s_containerHalfY,
                              MagnetFactory::s_containerHalfZ};

/// z from the GDML reference.
inline constexpr Slot kTimingDetector{"/SHiP/timing_detector",
                                      7,
                                      detail::zero,
                                      detail::zero,
                                      95.902 * detail::m,
                                      TimingDetectorFactory::s_containerHalfX,
                                      TimingDetectorFactory::s_containerHalfY,
                                      TimingDetectorFactory::s_containerHalfZ};

inline constexpr Slot kCalorimeter{"/SHiP/calorimeter",   8,
                                   detail::zero,          detail::zero,
                                   98.32 * detail::m,     Calo::kContainerHalfX,
                                   Calo::kContainerHalfY, Calo::kContainerHalfZ};

/// All subsystems, in placement order (upstream to downstream).
inline constexpr std::array kSlots{kTarget,         kMuonShield,     kNeutrinoDetector,
                                   kUpstreamTagger, kDecayVolume,    kTrackers,
                                   kMagnet,         kTimingDetector, kCalorimeter};

/// Pairs of containers that overlap on purpose.
inline constexpr std::array<std::array<std::string_view, 2>, 2> kAllowedOverlaps{{
    {kMuonShield.path, kNeutrinoDetector.path},
    {kTrackers.path, kMagnet.path},
}};

// ── Layout predicates ───────────────────────────────────────────────────

/// Centres never decrease along the slot list.
template <std::size_t N>
[[nodiscard]] constexpr std::size_t firstOutOfOrder(const std::array<Slot, N>& slots) {
    return checks::firstPairFailure(slots, [](const Slot& a, const Slot& b) { return a.z <= b.z; });
}

template <std::size_t M>
[[nodiscard]] constexpr bool isAllowedOverlap(
    const std::array<std::array<std::string_view, 2>, M>& allowed, const Slot& a, const Slot& b) {
    for (const auto& pair : allowed) {
        if ((pair[0] == a.path && pair[1] == b.path) || (pair[0] == b.path && pair[1] == a.path)) {
            return true;
        }
    }
    return false;
}

/// Index of the first slot that overlaps a later one without being on the
/// allowlist, or npos.
template <std::size_t N, std::size_t M = kAllowedOverlaps.size()>
[[nodiscard]] constexpr std::size_t firstUnexpectedOverlap(
    const std::array<Slot, N>& slots,
    const std::array<std::array<std::string_view, 2>, M>& allowed = kAllowedOverlaps) {
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = i + 1; j < N; ++j) {
            if (checks::overlaps(slots[i].box(), slots[j].box()) &&
                !isAllowedOverlap(allowed, slots[i], slots[j])) {
                return i;
            }
        }
    }
    return checks::npos;
}

/// The world volume built by CavernFactory, centred on the origin.
inline constexpr checks::Box kWorld = checks::box(
    CavernFactory::s_worldHalfX, CavernFactory::s_worldHalfY, CavernFactory::s_worldHalfZ);

/// Container lies inside the envelope along the beam (with @p tol of slack,
/// for values that are equal on paper but rounded differently).
[[nodiscard]] constexpr bool fitsEnvelopeZ(const Slot& slot, const Envelopes::Envelope& env,
                                           LengthMm tol = 1e-6 * detail::mm) {
    return checks::contains(env.z(), slot.zSpan(), tol);
}

/// Container stays inside the envelope's largest transverse size.
[[nodiscard]] constexpr bool fitsEnvelopeXY(const Slot& slot, const Envelopes::Envelope& env) {
    const auto xReach = (slot.x < detail::zero ? -slot.x : slot.x) + slot.halfX;
    const auto yReach = (slot.y < detail::zero ? -slot.y : slot.y) + slot.halfY;
    return xReach <= env.maxHalfWidth() && yReach <= env.maxHalfHeight();
}

// ── Layout checks ───────────────────────────────────────────────────────

static_assert(firstOutOfOrder(kSlots) == checks::npos,
              "subsystem centres must not decrease along the beam");
static_assert(firstUnexpectedOverlap(kSlots) == checks::npos,
              "two subsystem containers overlap and are not in kAllowedOverlaps");
static_assert(checks::firstFailure(kSlots,
                                   [](const Slot& s) {
                                       return checks::contains(kWorld, s.box());
                                   }) == checks::npos,
              "a subsystem container reaches outside the world volume");
static_assert(checks::firstFailure(kAllowedOverlaps,
                                   [](const auto& pair) {
                                       for (const auto& a : kSlots) {
                                           for (const auto& b : kSlots) {
                                               if (a.path == pair[0] && b.path == pair[1]) {
                                                   return checks::overlaps(a.box(), b.box());
                                               }
                                           }
                                       }
                                       return false;
                                   }) == checks::npos,
              "an allowlisted pair no longer overlaps (or names an unknown subsystem): "
              "drop it from kAllowedOverlaps");

// ── Cavern ──────────────────────────────────────────────────────────────

/// The halls CavernFactory cuts out of the rock, in the world frame. The
/// subsystems are siblings of the rock in the world, so each must sit in a
/// hall, or it overlaps solid rock.
inline constexpr std::array kCavities = [] {
    using C = CavernFactory;
    auto hall = [](LengthMm px, LengthMm py, LengthMm pz, LengthMm hx, LengthMm hy, LengthMm hz) {
        return checks::box(px, py, pz + C::s_cavernPosZ, hx, hy, hz);
    };
    return std::array{
        hall(C::s_muonCavernPosX, C::s_muonCavernPosY, C::s_muonCavernPosZ, C::s_muonCavernHalfX,
             C::s_muonCavernHalfY, C::s_muonCavernHalfZ),
        hall(C::s_expCavernPosX, C::s_expCavernPosY, C::s_expCavernPosZ, C::s_expCavernHalfX,
             C::s_expCavernHalfY, C::s_expCavernHalfZ),
        hall(C::s_stairPosX, C::s_stairPosY, C::s_stairPosZ, C::s_stairHalfX, C::s_stairHalfY,
             C::s_stairHalfZ),
        hall(C::s_yokePitPosX, C::s_yokePitPosY, C::s_yokePitPosZ, C::s_yokePitHalfX,
             C::s_yokePitHalfY, C::s_yokePitHalfZ),
        hall(C::s_targetPitPosX, C::s_targetPitPosY, C::s_targetPitPosZ, C::s_targetPitHalfX,
             C::s_targetPitHalfY, C::s_targetPitHalfZ),
    };
}();

/// The container lies entirely within one hall.
[[nodiscard]] constexpr bool isInCavity(const Slot& slot) {
    for (const auto& cavity : kCavities) {
        if (checks::contains(cavity, slot.box())) {
            return true;
        }
    }
    return false;
}

static_assert(checks::firstFailure(kSlots,
                                   [](const Slot& s) {
                                       return isInCavity(s) || s.path == kCalorimeter.path;
                                   }) == checks::npos,
              "a subsystem container reaches into the cavern rock");
// Known deviation: the cavern was taken from GDML with a different z origin,
// and the calorimeter reaches ~442 mm past the end of the experiment hall.
static_assert(!isInCavity(kCalorimeter),
              "the calorimeter now fits the experiment hall: drop its exception above");

// ── EDMS envelopes ──────────────────────────────────────────────────────

namespace detail {
/// Muon shield region: the hadron stopper start to the end of the last
/// magnet. The hadron stopper has no transverse size in the CSV, so the
/// transverse limits come from the magnets alone.
inline constexpr Envelopes::Envelope kMuonShieldRegion = [] {
    auto env = Envelopes::hull("Muon shield region", Envelopes::kMuonShield);
    env.zStart = Envelopes::kHadronStopper.zStart;
    return env;
}();

/// Decay volume plus the SBT shell wrapped around it.
inline constexpr Envelopes::Envelope kDecayVolumeWithSBT = [] {
    auto env = Envelopes::kDecayVolume;
    env.halfWidthUp = env.halfWidthUp + Envelopes::kSurroundTaggerThickness;
    env.halfWidthDown = env.halfWidthDown + Envelopes::kSurroundTaggerThickness;
    env.halfHeightUp = env.halfHeightUp + Envelopes::kSurroundTaggerThickness;
    env.halfHeightDown = env.halfHeightDown + Envelopes::kSurroundTaggerThickness;
    return env;
}();

/// Tracker stations 1 to 4, including the magnet gap between 2 and 3.
inline constexpr auto kTrackerRegion = Envelopes::hull("Trackers", Envelopes::kTrackerStations);

/// Split ECAL front and back plus the HCAL.
inline constexpr auto kCalorimeterRegion = Envelopes::hull("Calorimeter", Envelopes::kCalorimeter);
}  // namespace detail

static_assert(kTarget.zSpan().hi <= Envelopes::kHadronStopper.zStart,
              "the target vacuum box reaches into the hadron stopper");

static_assert(fitsEnvelopeXY(kMuonShield, detail::kMuonShieldRegion),
              "the muon shield container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeZ(kNeutrinoDetector, Envelopes::kNeutrinoDetector),
              "the SND container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeXY(kNeutrinoDetector, Envelopes::kNeutrinoDetector),
              "the SND container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeZ(kUpstreamTagger, Envelopes::kUpstreamTagger),
              "the UBT container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeZ(kDecayVolume, Envelopes::kDecayVolume),
              "the decay volume container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeXY(kDecayVolume, detail::kDecayVolumeWithSBT),
              "the decay volume container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeZ(kTrackers, detail::kTrackerRegion),
              "the trackers container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeXY(kTrackers, detail::kTrackerRegion),
              "the trackers container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeZ(kMagnet, Envelopes::kMagnet),
              "the magnet container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeXY(kMagnet, Envelopes::kMagnet),
              "the magnet container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeXY(kTimingDetector, Envelopes::kTimingDetector),
              "the timing detector container is outside its EDMS envelope transversely");
static_assert(fitsEnvelopeZ(kCalorimeter, detail::kCalorimeterRegion),
              "the calorimeter container is outside its EDMS envelope along z");
static_assert(fitsEnvelopeXY(kCalorimeter, detail::kCalorimeterRegion),
              "the calorimeter container is outside its EDMS envelope transversely");

// Known deviations from the EDMS envelopes. Each is asserted to still be a
// deviation, so whoever fixes one is told to turn it into a positive check.
static_assert(!fitsEnvelopeZ(kMuonShield, detail::kMuonShieldRegion),
              "the muon shield container now fits the EDMS region: assert fitsEnvelopeZ instead");
static_assert(!fitsEnvelopeXY(kUpstreamTagger, Envelopes::kUpstreamTagger),
              "the UBT container now fits the EDMS envelope: assert fitsEnvelopeXY instead");
static_assert(!fitsEnvelopeZ(kTimingDetector, Envelopes::kTimingDetector),
              "the timing detector now fits the EDMS envelope: assert fitsEnvelopeZ instead");

// ── Trackers against their neighbours ──────────────────────────────────

/// Absolute z-span of one tracker station.
[[nodiscard]] constexpr checks::Span trackerStation(LengthMm centreZ) {
    return checks::span(centreZ, TrackersFactory::s_halfZ);
}

inline constexpr std::array kTrackerStationZ{
    TrackersFactory::s_station1Z, TrackersFactory::s_station2Z, TrackersFactory::s_station3Z,
    TrackersFactory::s_station4Z};
static_assert(kTrackerStationZ.size() == Envelopes::kTrackerStations.size());
static_assert(
    [] {
        for (std::size_t i = 0; i < kTrackerStationZ.size(); ++i) {
            if (!checks::contains(Envelopes::kTrackerStations[i].z(),
                                  trackerStation(kTrackerStationZ[i]), 1e-6 * detail::mm)) {
                return false;
            }
        }
        return true;
    }(),
    "a tracker station is outside its EDMS envelope");
static_assert(trackerStation(TrackersFactory::s_station2Z).hi <= kMagnet.zSpan().lo &&
                  kMagnet.zSpan().hi <= trackerStation(TrackersFactory::s_station3Z).lo,
              "the spectrometer magnet must sit between tracker stations 2 and 3");
static_assert(checks::contains({trackerStation(TrackersFactory::s_station2Z).hi,
                                kMagnet.zSpan().lo},
                               checks::span(TrackersFactory::s_trackerMagnetZ,
                                            TrackersFactory::s_trackerMagnetHalfZ)),
              "the tracker-magnet marker must sit in the gap between station 2 and the magnet");

}  // namespace SHiPGeometry::Layout
