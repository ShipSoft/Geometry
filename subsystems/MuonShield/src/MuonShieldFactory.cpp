// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/MuonShield/MuonShieldFactory.h"

#include "SHiP/geometry/Materials.h"
#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/SubsystemRegistry.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace SHiP::geometry {

using units::gm;
using units::mm;

// ── Compile-time validation ──────────────────────────────────────────────
namespace {

using MSF = MuonShieldFactory;

constexpr auto kZero = 0.0 * mm;

constexpr checks::Box magnetBox(const MSF::MagnetData& magnet) {
    return checks::box(kZero, kZero, magnet.zCentre(), magnet.halfX, magnet.halfY, magnet.halfZ);
}

constexpr checks::Box areaBox() {
    return checks::box(kZero, kZero, MSF::s_centreZ, MSF::s_areaHalfX, MSF::s_areaHalfY,
                       MSF::s_areaHalfZ);
}

constexpr checks::Box cavityBox(const MSF::CavitySection& section) {
    return checks::box(kZero, kZero, 0.5 * (section.zStart + section.zEnd), section.halfX,
                       section.halfY, 0.5 * (section.zEnd - section.zStart));
}

/// Only M5 and M6 carry the SND, and every cavity step stays inside the iron
/// of the magnets it cuts transversely, so each keeps an iron frame around it.
constexpr bool sndCavityInM5AndM6() {
    return !MSF::hasSndCavity(MSF::k_hadronStopper) &&
           std::ranges::all_of(MSF::k_magnets, [](const MSF::MagnetData& magnet) {
               const std::string_view name = magnet.name;
               const bool expected = name == "magn_5" || name == "magn_6";
               if (MSF::hasSndCavity(magnet) != expected) {
                   return false;
               }
               return std::ranges::all_of(MSF::k_sndCavity, [&magnet](const MSF::CavitySection& s) {
                   return !MSF::cuts(s, magnet) ||
                          (s.halfX < magnet.halfX && s.halfY < magnet.halfY);
               });
           });
}

}  // namespace

static_assert(sndCavityInM5AndM6(),
              "the SND cavity must cut M5 and M6 only, and leave iron around it transversely");
static_assert(std::ranges::all_of(MSF::k_sndCavity,
                                  [](const MSF::CavitySection& s) {
                                      return checks::contains(areaBox(), cavityBox(s));
                                  }),
              "the SND cavity sticks out of the muon-shield container");
static_assert(MSF::k_sndCavity.back().zStart == MSF::k_magnets[5].zStart &&
                  MSF::k_sndCavity.back().zEnd == MSF::k_magnets[5].zEnd(),
              "the HCAL step of the SND cavity must span M6 face to face");
static_assert(checks::contains(areaBox(), magnetBox(MSF::k_hadronStopper)),
              "the hadron stopper sticks out of the muon-shield container");
static_assert(checks::firstFailure(MSF::k_magnets,
                                   [](const MSF::MagnetData& magnet) {
                                       return checks::contains(areaBox(), magnetBox(magnet));
                                   }) == checks::npos,
              "a muon-shield magnet sticks out of the muon-shield container");
static_assert(MSF::k_hadronStopper.zEnd() <= MSF::k_magnets.front().zStart,
              "the hadron stopper must end before M1");
static_assert(checks::firstPairFailure(MSF::k_magnets,
                                       [](const MSF::MagnetData& a, const MSF::MagnetData& b) {
                                           return a.zEnd() <= b.zStart;
                                       }) == checks::npos,
              "muon-shield magnets must be ordered along z without overlapping");

// ---------------------------------------------------------------------------

namespace {

/// @p body, a solid on the beam axis from @p zLo to @p zHi (world frame),
/// with the part of the SND cavity inside it cut out. Where a cut reaches a
/// face of the body, or a wider step of the cavity, it is extended a little
/// past it, so the subtraction leaves no zero-thickness skin. Where the cavity
/// ends inside the body, the cut ends exactly on the SND's face.
const GeoShape* subtractSndCavity(const GeoShape* body, units::LengthMm zLo, units::LengthMm zHi) {
    using Section = MuonShieldFactory::CavitySection;
    const auto& cavity = MuonShieldFactory::k_sndCavity;
    constexpr auto eps = 1.0 * mm;
    const auto wider = [&cavity](std::size_t i, const Section& s) {
        if (i >= cavity.size()) {
            return false;
        }
        const auto& n = cavity.at(i);
        return n.halfX >= s.halfX && n.halfY >= s.halfY;
    };
    const auto zCentre = 0.5 * (zLo + zHi);
    const GeoShape* shape = body;
    for (std::size_t i = 0; i < cavity.size(); ++i) {
        const Section& section = cavity.at(i);
        if (section.zEnd <= zLo || zHi <= section.zStart) {
            continue;
        }
        auto cutLo = section.zStart;
        if (cutLo <= zLo) {
            cutLo = zLo - eps;
        } else if (i > 0 && wider(i - 1, section)) {
            cutLo -= eps;
        }
        auto cutHi = section.zEnd;
        if (cutHi >= zHi) {
            cutHi = zHi + eps;
        } else if (wider(i + 1, section)) {
            cutHi += eps;
        }
        auto const* cut =
            new GeoBox(gm(section.halfX), gm(section.halfY), gm(0.5 * (cutHi - cutLo)));
        GeoTrf::Translate3D const cutTrf(0.0, 0.0, gm(0.5 * (cutLo + cutHi) - zCentre));
        shape = &shape->subtract(*cut << cutTrf);
    }
    return shape;
}

}  // namespace

MuonShieldFactory::MuonShieldFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* MuonShieldFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    // The container has the SND's shape cut out of it, so that the SND, its
    // sibling in the world, does not overlap it (Layout.h, kCutOuts).
    const GeoShape* area = subtractSndCavity(
        new GeoBox(gm(s_areaHalfX), gm(s_areaHalfY), gm(s_areaHalfZ)), s_zStart, s_zEnd);
    auto const* areaLog = new GeoLogVol("/SHiP/muon_shield", area, air);
    auto* areaPhys = new GeoPhysVol(areaLog);

    // The hadron stopper first, then M1-M7, each with its index as identifier.
    int id = 0;
    const auto place = [&](const MagnetData& magnet) {
        const GeoShape* shape = new GeoBox(gm(magnet.halfX), gm(magnet.halfY), gm(magnet.halfZ));
        shape = subtractSndCavity(shape, magnet.zStart, magnet.zEnd());

        const std::string name = "/SHiP/muon_shield/" + std::string(magnet.name);
        auto* magnetPhys = new GeoPhysVol(new GeoLogVol(name, shape, iron));
        areaPhys->add(new GeoNameTag(name));
        areaPhys->add(new GeoIdentifierTag(id++));
        areaPhys->add(
            new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, gm(magnet.zCentre() - s_centreZ))));
        areaPhys->add(magnetPhys);
    };
    place(k_hadronStopper);
    for (const MagnetData& magnet : k_magnets) {
        place(magnet);
    }

    return areaPhys;
}

// Static self-registration, and a link anchor the generated builder code calls.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization,cert-err58-cpp,misc-use-internal-linkage)
REGISTER_SUBSYSTEM(MuonShieldFactory)

}  // namespace SHiP::geometry
