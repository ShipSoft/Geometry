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
    return checks::box(kZero, kZero, magnet.zStart + magnet.halfZ, magnet.halfX, magnet.halfY,
                       magnet.halfZ);
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
constexpr bool sndHoleInM5AndM6() {
    return std::ranges::all_of(MSF::k_magnets, [](const MSF::MagnetData& magnet) {
        const std::string_view name = magnet.name;
        const bool expected = name == "magn_5" || name == "magn_6";
        if (MSF::hasSndHole(magnet) != expected) {
            return false;
        }
        return std::ranges::all_of(MSF::k_sndCavity, [&magnet](const MSF::CavitySection& s) {
            return !MSF::cuts(s, magnet) || (s.halfX < magnet.halfX && s.halfY < magnet.halfY);
        });
    });
}

/// The cavity steps follow one another along z without gaps.
constexpr bool cavityIsContiguous() {
    for (std::size_t i = 1; i < MSF::k_sndCavity.size(); ++i) {
        if (MSF::k_sndCavity.at(i).zStart != MSF::k_sndCavity.at(i - 1).zEnd) {
            return false;
        }
    }
    return true;
}

}  // namespace

static_assert(checks::firstFailure(MSF::k_magnets,
                                   [](const MSF::MagnetData& magnet) {
                                       return checks::contains(areaBox(), magnetBox(magnet));
                                   }) == checks::npos,
              "a muon-shield magnet sticks out of the MuonShieldArea container");
static_assert(checks::firstPairFailure(MSF::k_magnets,
                                       [](const MSF::MagnetData& a, const MSF::MagnetData& b) {
                                           return magnetBox(a).z.hi <= magnetBox(b).z.lo;
                                       }) == checks::npos,
              "muon-shield magnets must be ordered along z without overlapping");
static_assert(MSF::k_magnets.front().zStart == MSF::s_zStart &&
                  MSF::k_magnets.back().zStart + 2 * MSF::k_magnets.back().halfZ == MSF::s_zEnd,
              "the MuonShieldArea container must span M1's upstream to M7's downstream face");
static_assert(sndHoleInM5AndM6(),
              "the SND hole must cut M5 and M6 only, and leave iron around it transversely");
static_assert(cavityIsContiguous(), "the SND cavity steps must follow one another along z");
static_assert(std::ranges::all_of(MSF::k_sndCavity,
                                  [](const MSF::CavitySection& s) {
                                      return checks::contains(areaBox(), cavityBox(s));
                                  }),
              "the SND cavity sticks out of the MuonShieldArea container");
static_assert(MSF::k_sndCavity.back().zEnd ==
                  MSF::k_magnets[5].zStart + 2 * MSF::k_magnets[5].halfZ,
              "the SND cavity must end at M6's downstream face");
static_assert(MSF::k_sndCavity.back().zStart == MSF::k_magnets[5].zStart,
              "the HCAL step of the SND cavity must start at M6's upstream face");

// ---------------------------------------------------------------------------

MuonShieldFactory::MuonShieldFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* MuonShieldFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto const* area = new GeoBox(gm(s_areaHalfX), gm(s_areaHalfY), gm(s_areaHalfZ));
    auto const* areaLog = new GeoLogVol("/SHiP/muon_shield", area, air);
    auto* areaPhys = new GeoPhysVol(areaLog);

    for (const MagnetData& magnet : k_magnets) {
        const auto zLo = magnet.zStart;
        const auto zHi = magnet.zStart + 2 * magnet.halfZ;
        const auto zCentre = magnet.zStart + magnet.halfZ;

        const GeoShape* shape = new GeoBox(gm(magnet.halfX), gm(magnet.halfY), gm(magnet.halfZ));
        // Cut each cavity step over the part of it inside this magnet. Where a
        // cut reaches a magnet face, or a wider step of the cavity, it is
        // extended a little past it, so the subtraction leaves no
        // zero-thickness iron skin.
        constexpr auto eps = 1.0 * mm;
        const auto wider = [](std::size_t i, const CavitySection& s) {
            if (i >= k_sndCavity.size()) {
                return false;
            }
            const auto& n = k_sndCavity.at(i);
            return n.halfX >= s.halfX && n.halfY >= s.halfY;
        };
        for (std::size_t i = 0; i < k_sndCavity.size(); ++i) {
            const CavitySection& section = k_sndCavity.at(i);
            if (!cuts(section, magnet)) {
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

        const std::string name = "/SHiP/muon_shield/" + std::string(magnet.name);
        auto* magnetPhys = new GeoPhysVol(new GeoLogVol(name, shape, iron));
        areaPhys->add(new GeoNameTag(name));
        areaPhys->add(new GeoIdentifierTag(static_cast<int>(&magnet - k_magnets.data())));
        areaPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, gm(zCentre - s_centreZ))));
        areaPhys->add(magnetPhys);
    }

    return areaPhys;
}

REGISTER_SUBSYSTEM(MuonShieldFactory)

}  // namespace SHiP::geometry
