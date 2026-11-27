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
#include <GeoModelKernel/GeoTransform.h>

#include <string>

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

}  // namespace

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

MuonShieldFactory::MuonShieldFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* MuonShieldFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");

    auto const* area = new GeoBox(gm(s_areaHalfX), gm(s_areaHalfY), gm(s_areaHalfZ));
    auto const* areaLog = new GeoLogVol("/SHiP/muon_shield", area, air);
    auto* areaPhys = new GeoPhysVol(areaLog);

    // The hadron stopper first, then M1-M7, each with its index as identifier.
    int id = 0;
    const auto place = [&](const MagnetData& magnet) {
        auto const* box = new GeoBox(gm(magnet.halfX), gm(magnet.halfY), gm(magnet.halfZ));
        const std::string name = "/SHiP/muon_shield/" + std::string(magnet.name);
        auto* magnetPhys = new GeoPhysVol(new GeoLogVol(name, box, iron));
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
