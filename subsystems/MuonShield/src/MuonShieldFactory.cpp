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

constexpr checks::Box sndHoleBox() {
    return checks::box(kZero, kZero, 0.5 * (MSF::s_sndHoleZStart + MSF::s_sndHoleZEnd),
                       MSF::s_sndHoleHalfX, MSF::s_sndHoleHalfY,
                       0.5 * (MSF::s_sndHoleZEnd - MSF::s_sndHoleZStart));
}

/// Only M5 and M6 carry the SND, and the hole stays inside their iron
/// transversely, so each keeps an iron frame around it.
constexpr bool sndHoleInM5AndM6() {
    return std::ranges::all_of(MSF::k_magnets, [](const MSF::MagnetData& magnet) {
        const std::string_view name = magnet.name;
        const bool expected = name == "magn_5" || name == "magn_6";
        if (MSF::hasSndHole(magnet) != expected) {
            return false;
        }
        return !expected ||
               (MSF::s_sndHoleHalfX < magnet.halfX && MSF::s_sndHoleHalfY < magnet.halfY);
    });
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
static_assert(checks::contains(areaBox(), sndHoleBox()),
              "the SND hole sticks out of the MuonShieldArea container");
static_assert(MSF::s_sndHoleZEnd == MSF::k_magnets[5].zStart + 2 * MSF::k_magnets[5].halfZ,
              "the SND hole must end at M6's downstream face");

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
        if (hasSndHole(magnet)) {
            // The cut runs over the part of the hole inside this magnet. Where
            // it reaches a magnet face it is extended a little past it, so the
            // subtraction leaves no zero-thickness iron skin.
            constexpr auto eps = 1.0 * mm;
            const auto cutLo = s_sndHoleZStart > zLo ? s_sndHoleZStart : zLo - eps;
            const auto cutHi = s_sndHoleZEnd < zHi ? s_sndHoleZEnd : zHi + eps;
            auto const* cut =
                new GeoBox(gm(s_sndHoleHalfX), gm(s_sndHoleHalfY), gm(0.5 * (cutHi - cutLo)));
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
