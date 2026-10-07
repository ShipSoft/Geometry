// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/Calorimeter/CalorimeterConstants.h"
#include "SHiP/geometry/Calorimeter/CalorimeterFactory.h"
#include "SHiP/geometry/Cavern/CavernFactory.h"
#include "SHiP/geometry/DecayVolume/DecayVolumeFactory.h"
#include "SHiP/geometry/DecayVolume/SBTConstants.h"
#include "SHiP/geometry/Envelopes.h"
#include "SHiP/geometry/Magnet/MagnetFactory.h"
#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"
#include "SHiP/geometry/NeutrinoDetector/NeutrinoDetectorFactory.h"
#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/Target/TargetFactory.h"
#include "SHiP/geometry/TimingDetector/TimingDetectorFactory.h"
#include "SHiP/geometry/Trackers/TrackersFactory.h"
#include "SHiP/geometry/Units.h"
#include "SHiP/geometry/UpstreamTagger/UpstreamTaggerFactory.h"

#include <array>
#include <string_view>

/**
 * @brief Where each subsystem container sits in the world volume.
 *
 * Each slot pairs a subsystem's registered placement (its descriptor(), which
 * the assembler places it from) with its container half-lengths. The
 * static_asserts below check the layout as a whole: ordering along the beam,
 * no overlaps beyond the known ones, everything inside the world, and every
 * container inside its EDMS envelope (Envelopes.h). Container half-lengths
 * are read from the factories, so resizing a container is checked here as
 * well. Known deviations are bounded: each fails the build both when it is
 * fixed and when it gets worse.
 *
 * Coordinates are in the world frame, whose origin is the front face of the
 * first target disk.
 */
namespace SHiP::geometry::Layout {

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
inline constexpr auto zero = 0.0 * units::mm;
}  // namespace detail

/// A factory's slot: its registered placement (descriptor()) together with
/// its container half-lengths.
template <typename Factory>
[[nodiscard]] constexpr Slot slotOf(LengthMm halfX, LengthMm halfY, LengthMm halfZ) {
    constexpr SubsystemDescriptor d = Factory::descriptor();
    return {d.node, d.id, d.x_mm * units::mm, d.y_mm * units::mm, d.z_mm * units::mm, halfX,
            halfY,  halfZ};
}

inline constexpr Slot kTarget =
    slotOf<TargetFactory>(TargetFactory::s_vacuumBoxHalfX, TargetFactory::s_vacuumBoxHalfY,
                          TargetFactory::s_vacuumBoxHalfZ);

inline constexpr Slot kMuonShield = slotOf<MuonShieldFactory>(
    MuonShieldFactory::s_areaHalfX, MuonShieldFactory::s_areaHalfY, MuonShieldFactory::s_areaHalfZ);

/// z 26.40-31.50 m (WARM muon-shield configuration). The SND sits within the
/// downstream end of the muon-shield region, so the two containers overlap
/// (known deviation #117, see kKnownOverlaps).
inline constexpr Slot kNeutrinoDetector = slotOf<NeutrinoDetectorFactory>(
    NeutrinoDetectorFactory::s_halfX, NeutrinoDetectorFactory::s_halfY,
    NeutrinoDetectorFactory::s_halfZ);

inline constexpr Slot kUpstreamTagger = slotOf<UpstreamTaggerFactory>(
    UpstreamTaggerFactory::s_halfX, UpstreamTaggerFactory::s_halfY, UpstreamTaggerFactory::s_halfZ);

inline constexpr Slot kDecayVolume =
    slotOf<DecayVolumeFactory>(SBT::kEnvelopeHalfX, SBT::kEnvelopeHalfY, SBT::kEnvelopeHalfZ);

/// The factory positions its stations relative to s_containerCentreZ, so the
/// container goes there and the stations land on their absolute z. The
/// container spans the magnet (stations 1-2 upstream, 3-4 downstream).
inline constexpr Slot kTrackers = slotOf<TrackersFactory>(
    TrackersFactory::s_halfX, TrackersFactory::s_halfY, TrackersFactory::s_containerHalfZ);

inline constexpr Slot kMagnet =
    slotOf<MagnetFactory>(MagnetFactory::s_containerHalfX, MagnetFactory::s_containerHalfY,
                          MagnetFactory::s_containerHalfZ);

inline constexpr Slot kTimingDetector = slotOf<TimingDetectorFactory>(
    TimingDetectorFactory::s_containerHalfX, TimingDetectorFactory::s_containerHalfY,
    TimingDetectorFactory::s_containerHalfZ);

inline constexpr Slot kCalorimeter =
    slotOf<CalorimeterFactory>(Calo::kContainerHalfX, Calo::kContainerHalfY, Calo::kContainerHalfZ);

/// All subsystems, in placement order (upstream to downstream).
inline constexpr std::array kSlots{kTarget,         kMuonShield,     kNeutrinoDetector,
                                   kUpstreamTagger, kDecayVolume,    kTrackers,
                                   kMagnet,         kTimingDetector, kCalorimeter};

/// Two containers that overlap, although Geant4 does not support overlapping
/// siblings. Known deviations, each tracked in an issue: the overlap must lie
/// within @c region (world frame, mm), so the build fails both when one is
/// fixed and when one grows.
struct KnownOverlap {
    std::string_view a;
    std::string_view b;
    checks::Box region;
};

inline constexpr std::array kKnownOverlaps{
    // #117: the SND sits in the downstream end of the muon shield.
    KnownOverlap{.a = kMuonShield.path,
                 .b = kNeutrinoDetector.path,
                 .region = checks::bounds(-400, 400, -400, 400, 26400, 31487.3)},
    // #118: the trackers container spans the whole magnet along the beam.
    KnownOverlap{.a = kTrackers.path,
                 .b = kMagnet.path,
                 .region = checks::bounds(-3000, 3000, -3430, 3430, 87070, 92070)},
};

// ── Layout predicates ───────────────────────────────────────────────────

/// Centres never decrease along the slot list.
template <std::size_t N>
[[nodiscard]] constexpr std::size_t firstOutOfOrder(const std::array<Slot, N>& slots) {
    return checks::firstPairFailure(slots, [](const Slot& a, const Slot& b) { return a.z <= b.z; });
}

/// The pair is listed in @p known and overlaps no further than its region.
template <std::size_t M>
[[nodiscard]] constexpr bool isKnownOverlap(const std::array<KnownOverlap, M>& known, const Slot& a,
                                            const Slot& b) {
    for (const auto& k : known) {
        if ((k.a == a.path && k.b == b.path) || (k.a == b.path && k.b == a.path)) {
            return checks::overlapsWithin(a.box(), b.box(), k.region);
        }
    }
    return false;
}

/// Index of the first slot that overlaps a later one, unless the pair is a
/// known overlap that has not grown; npos if there is none.
template <std::size_t N, std::size_t M = kKnownOverlaps.size()>
[[nodiscard]] constexpr std::size_t firstUnexpectedOverlap(
    const std::array<Slot, N>& slots, const std::array<KnownOverlap, M>& known = kKnownOverlaps) {
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = i + 1; j < N; ++j) {
            if (checks::overlaps(slots[i].box(), slots[j].box()) &&
                !isKnownOverlap(known, slots[i], slots[j])) {
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

/// Container stays inside the envelope's largest transverse size (with
/// @p tolX and @p tolY of slack).
[[nodiscard]] constexpr bool fitsEnvelopeXY(const Slot& slot, const Envelopes::Envelope& env,
                                            LengthMm tolX = detail::zero,
                                            LengthMm tolY = detail::zero) {
    const auto xReach = (slot.x < detail::zero ? -slot.x : slot.x) + slot.halfX;
    const auto yReach = (slot.y < detail::zero ? -slot.y : slot.y) + slot.halfY;
    return xReach <= env.maxHalfWidth() + tolX && yReach <= env.maxHalfHeight() + tolY;
}

// ── Layout checks ───────────────────────────────────────────────────────

static_assert(firstOutOfOrder(kSlots) == checks::npos,
              "subsystem centres must not decrease along the beam");
static_assert(firstUnexpectedOverlap(kSlots) == checks::npos,
              "two subsystem containers overlap, and the pair is not in kKnownOverlaps or "
              "overlaps beyond its recorded region");
static_assert(checks::firstFailure(kSlots,
                                   [](const Slot& s) {
                                       return checks::contains(kWorld, s.box());
                                   }) == checks::npos,
              "a subsystem container reaches outside the world volume");
static_assert(checks::firstFailure(kKnownOverlaps,
                                   [](const KnownOverlap& known) {
                                       for (const auto& a : kSlots) {
                                           for (const auto& b : kSlots) {
                                               if (a.path == known.a && b.path == known.b) {
                                                   return checks::overlaps(a.box(), b.box());
                                               }
                                           }
                                       }
                                       return false;
                                   }) == checks::npos,
              "a known overlap is gone (or names an unknown subsystem): "
              "drop it from kKnownOverlaps and close its issue");

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

/// The container lies within the halls taken together: the muon shield runs
/// from TCC8 across the step into ECN3, and the magnet sits partly in the
/// yoke pit.
[[nodiscard]] constexpr bool isInCavity(const Slot& slot) {
    return checks::coveredBy(slot.box(), kCavities);
}

static_assert(checks::firstFailure(kSlots,
                                   [](const Slot& s) {
                                       return isInCavity(s) || s.path == kTrackers.path ||
                                              s.path == kCalorimeter.path;
                                   }) == checks::npos,
              "a subsystem container reaches into the cavern rock");
// Known deviations (#120): the ECN3 floor is 3.364 m below the beam, and two
// containers centred on the beam are taller than that allows outside the yoke
// pit. The tracker container (half-height 3.43 m) reaches 66 mm into the
// floor at stations 1 and 4; the calorimeter container (half-height 3.5 m, the
// EDMS envelope) reaches 136 mm into it, although its modules fit.
inline constexpr auto kTrackersIntoFloor = 66.0 * detail::mm;
inline constexpr auto kCalorimeterIntoFloor = 136.0 * detail::mm;
static_assert(!isInCavity(kTrackers),
              "the tracker container now fits the experiment hall: drop its exception above");
static_assert(checks::coveredBy(checks::shrunk(kTrackers.box(), kTrackersIntoFloor), kCavities),
              "the tracker container reaches further into the rock than kTrackersIntoFloor");
static_assert(!isInCavity(kCalorimeter),
              "the calorimeter container now fits the experiment hall: drop its exception above");
static_assert(checks::coveredBy(checks::shrunk(kCalorimeter.box(), kCalorimeterIntoFloor),
                                kCavities),
              "the calorimeter container reaches further into the rock than "
              "kCalorimeterIntoFloor");

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

// Known deviations from the EDMS envelopes (#120). Each is bounded by its
// current size: if one is fixed, assert the positive check above instead; if
// one gets worse, the second assert fails.
inline constexpr auto kMuonShieldEarlyStart = 101.0 * detail::mm;  // 100.7 mm
inline constexpr auto kUpstreamTaggerExcessX = 1450.0 * detail::mm;
inline constexpr auto kUpstreamTaggerExcessY = 1600.0 * detail::mm;
inline constexpr auto kTimingDetectorEarlyStart = 168.0 * detail::mm;
static_assert(!fitsEnvelopeZ(kMuonShield, detail::kMuonShieldRegion),
              "the muon shield container now fits the EDMS region: assert fitsEnvelopeZ instead");
static_assert(fitsEnvelopeZ(kMuonShield, detail::kMuonShieldRegion, kMuonShieldEarlyStart),
              "the muon shield container deviates from the EDMS region by more than "
              "kMuonShieldEarlyStart");
static_assert(!fitsEnvelopeXY(kUpstreamTagger, Envelopes::kUpstreamTagger),
              "the UBT container now fits the EDMS envelope: assert fitsEnvelopeXY instead");
static_assert(fitsEnvelopeXY(kUpstreamTagger, Envelopes::kUpstreamTagger, kUpstreamTaggerExcessX,
                             kUpstreamTaggerExcessY),
              "the UBT container exceeds the EDMS envelope by more than "
              "kUpstreamTaggerExcessX/Y");
static_assert(!fitsEnvelopeZ(kTimingDetector, Envelopes::kTimingDetector),
              "the timing detector now fits the EDMS envelope: assert fitsEnvelopeZ instead");
static_assert(fitsEnvelopeZ(kTimingDetector, Envelopes::kTimingDetector, kTimingDetectorEarlyStart),
              "the timing detector deviates from the EDMS envelope by more than "
              "kTimingDetectorEarlyStart");

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

}  // namespace SHiP::geometry::Layout
