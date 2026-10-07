// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/DecayVolume/DecayVolumeFactory.h"
#include "SHiP/geometry/DecayVolume/SBTConstants.h"
#include "SHiP/geometry/DecayVolume/SBTEnvelope.h"
#include "SHiP/geometry/DecayVolume/SBTSensorBuilder.h"
#include "SHiP/geometry/DecayVolume/SBTStructureBuilder.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <array>
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using SHiP::geometry::Materials;
namespace SBT = SHiP::geometry::SBT;

namespace {
// The SBT is placed flat (every child of the container is a leaf), so the
// shapes of the direct children fully describe the geometry: GeoBox = steel
// structure, GeoTrap = helium + sensor walls/cells.
struct ChildShapeCounts {
    unsigned total = 0;
    unsigned boxes = 0;
    unsigned traps = 0;
    unsigned helium = 0;
};

ChildShapeCounts countByShape(const GeoVPhysVol* vol) {
    ChildShapeCounts c;
    c.total = vol->getNChildVols();
    for (unsigned int i = 0; i < vol->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*vol->getChildVol(i);
        const GeoLogVol* lv = child->getLogVol();
        const GeoShape* shape = lv->getShape();
        if (dynamic_cast<const GeoBox*>(shape))
            ++c.boxes;
        if (dynamic_cast<const GeoTrap*>(shape))
            ++c.traps;
        if (lv->getName().find("helium") != std::string::npos)
            ++c.helium;
    }
    return c;
}
}  // namespace

// The container is an air box enclosing the SBT structure + sensors and the
// central helium frustum.
// CSV limits: DecayVolume halfX <= 2200, halfY <= 3300, halfZ <= 25200
TEST_CASE("DecayVolumeWithinEnvelope", "[decayvolume]") {
    Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    auto* box = dynamic_cast<const GeoBox*>(dv->getLogVol()->getShape());
    REQUIRE(box != nullptr);
    CHECK(box->getXHalfLength() <= 2200.0);
    CHECK(box->getYHalfLength() <= 3300.0);
    CHECK(box->getZHalfLength() <= 25200.0);
}

// Steel H-beam structure: 66 column + 120 corner-beam + 60 longitudinal +
// 66 cross-beam GeoBox pieces = 312, all direct children of the container.
TEST_CASE("DecayVolumeStructureBoxCount", "[decayvolume]") {
    Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const ChildShapeCounts c = countByShape(dv);
    CHECK(c.boxes == 312u);  // NOLINT(readability/check)
}

// Sensor system: 130 containers, each Z-split into 2 pieces of 7 Al walls +
// 6 LAB cells (13 GeoTraps) -> 130*2*13 = 3380 sensor traps; plus the helium,
// which is 2 slabs per sub-frustum (20) rather than a single frustum, because
// the free region it fills is not linear in Z. 3380 + 20 = 3400 GeoTraps.
TEST_CASE("DecayVolumeSensorTrapCount", "[decayvolume]") {
    Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const ChildShapeCounts c = countByShape(dv);
    CHECK(c.traps == 3400u);  // NOLINT(readability/check)
    CHECK(c.helium == 20u);   // NOLINT(readability/check)
}

// Flat architecture: total direct children = 312 structure + 3380 sensors +
// 20 helium slabs = 3712, with no grandchildren.
TEST_CASE("DecayVolumeChildCount", "[decayvolume]") {
    Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    CHECK(dv->getNChildVols() == 3712u);  // NOLINT(readability/check)
}

// The central decay region is built from helium GeoTraps, derived from the
// innermost SBT surfaces so that they cannot overlap the structure or sensors.
TEST_CASE("DecayVolumeHasHeliumFrustum", "[decayvolume]") {
    Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const GeoVPhysVol* he = nullptr;
    for (unsigned int i = 0; i < dv->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*dv->getChildVol(i);
        if (child->getLogVol()->getName().find("helium") != std::string::npos) {
            he = child;
            break;
        }
    }
    REQUIRE(he != nullptr);
    auto* trap = dynamic_cast<const GeoTrap*>(he->getLogVol()->getShape());
    REQUIRE(trap != nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helium envelope regression tests.

//
// The point of these tests is that they assert a *property*, not a formula.
// They do not check that dx1 is 632.376 mm; they walk the geometry that the
// builders actually produced and check that no helium slab intersects any of
// it, and that the helium is flush against it. Re-parameterise the SBT — move
// a beam, change the flange width, resize the containers — and the tests still
// mean the right thing. That is the property PR #58's algebraic check lacked:
// it verified an expression, and the expression was an incomplete model of the
// geometry (it missed the flat-piece sawtooth in X, and the longitudinal beam
// flanges entirely).
//
// Overlap is decided by the separating-axis theorem. Every child of the decay
// volume is a GeoBox or a GeoTrap, i.e. a convex hexahedron, so SAT over the
// two shapes' face normals plus their pairwise edge cross-products is exact.

namespace {

using Vec = GeoTrf::Vector3D;

// A convex hexahedron: 8 world-space vertices, in the GeoTrap corner order
// (0-3 at -dz, 4-7 at +dz; within a face: -y-x, -y+x, +y-x, +y+x).
struct Hexa {
    std::string name;
    std::array<Vec, 8> v;
};

// The 6 quad faces and 12 edges of that corner ordering.
constexpr std::array<std::array<int, 3>, 6> kFaces = {
    {{0, 1, 3}, {4, 5, 7}, {0, 1, 5}, {2, 3, 7}, {0, 2, 6}, {1, 3, 7}}};
constexpr std::array<std::array<int, 2>, 12> kEdges = {{{0, 1},
                                                        {1, 3},
                                                        {3, 2},
                                                        {2, 0},
                                                        {4, 5},
                                                        {5, 7},
                                                        {7, 6},
                                                        {6, 4},
                                                        {0, 4},
                                                        {1, 5},
                                                        {2, 6},
                                                        {3, 7}}};

std::array<Vec, 8> boxVertices(const GeoBox& b) {
    const double hx = b.getXHalfLength(), hy = b.getYHalfLength(), hz = b.getZHalfLength();
    std::array<Vec, 8> v;
    int i = 0;
    for (int sz : {-1, 1})
        for (int sy : {-1, 1})
            for (int sx : {-1, 1})
                v[i++] = Vec(sx * hx, sy * hy, sz * hz);
    return v;
}

// The SAT axis set used by separation() (6 face normals, each from 3 vertices,
// plus 12 edge crosses) is exact only when the trap's quadrilateral faces are
// planar. Every trap this geometry builds has equal top/bottom half-widths
// (dxdyn == dxdyp) and zero shear (alpha == 0), which makes the faces planar.
// A future builder change that produced a genuinely sheared trap would leave
// the axis set incomplete, and separation() could then call an overlapping
// pair disjoint. Require planarity here so that regresses loudly instead of
// silently weakening the overlap guarantee.
void requirePlanarTrap(const GeoTrap& t) {
    REQUIRE(std::abs(t.getDxdyndzn() - t.getDxdypdzn()) < 1e-9);
    REQUIRE(std::abs(t.getDxdyndzp() - t.getDxdypdzp()) < 1e-9);
    REQUIRE(std::abs(t.getAngleydzn()) < 1e-12);
    REQUIRE(std::abs(t.getAngleydzp()) < 1e-12);
}

std::array<Vec, 8> trapVertices(const GeoTrap& t) {
    requirePlanarTrap(t);
    const double dz = t.getZHalfLength();
    const double tt = std::tan(t.getTheta());
    const double cx = tt * std::cos(t.getPhi());
    const double cy = tt * std::sin(t.getPhi());

    const std::array<double, 2> dy = {t.getDydzn(), t.getDydzp()};
    const std::array<double, 2> dxn = {t.getDxdyndzn(), t.getDxdyndzp()};
    const std::array<double, 2> dxp = {t.getDxdypdzn(), t.getDxdypdzp()};
    const std::array<double, 2> alp = {t.getAngleydzn(), t.getAngleydzp()};

    std::array<Vec, 8> v;
    int i = 0;
    for (int f = 0; f < 2; ++f) {
        const double s = (f == 0) ? -1.0 : +1.0;
        const double ox = s * dz * cx, oy = s * dz * cy, oz = s * dz;
        const double ta = std::tan(alp[f]);
        v[i++] = Vec(ox - dy[f] * ta - dxn[f], oy - dy[f], oz);
        v[i++] = Vec(ox - dy[f] * ta + dxn[f], oy - dy[f], oz);
        v[i++] = Vec(ox + dy[f] * ta - dxp[f], oy + dy[f], oz);
        v[i++] = Vec(ox + dy[f] * ta + dxp[f], oy + dy[f], oz);
    }
    return v;
}

// Separation of two convex hexahedra along the SAT axis set.
// > 0  => disjoint (and the value is a lower bound on their distance)
// <= 0 => they intersect
double separation(const Hexa& a, const Hexa& b) {
    double best = -std::numeric_limits<double>::infinity();

    auto probe = [&](const Vec& n) {
        const double nn = n.norm();
        if (nn < 1e-9)
            return;
        const Vec u = n / nn;
        double amin = std::numeric_limits<double>::infinity(), amax = -amin;
        double bmin = amin, bmax = amax;
        for (const Vec& p : a.v) {
            const double d = p.dot(u);
            amin = std::min(amin, d);
            amax = std::max(amax, d);
        }
        for (const Vec& p : b.v) {
            const double d = p.dot(u);
            bmin = std::min(bmin, d);
            bmax = std::max(bmax, d);
        }
        best = std::max(best, std::max(bmin - amax, amin - bmax));
    };

    for (const Hexa* h : {&a, &b})
        for (const auto& f : kFaces)
            probe((h->v[f[1]] - h->v[f[0]]).cross(h->v[f[2]] - h->v[f[0]]));

    for (const auto& ea : kEdges)
        for (const auto& eb : kEdges)
            probe((a.v[ea[1]] - a.v[ea[0]]).cross(b.v[eb[1]] - b.v[eb[0]]));

    return best;
}

// Collect every direct child of the decay volume as a world-space hexahedron.
// The SBT is placed flat, so all children are leaves.
void collect(const GeoVPhysVol* dv, std::vector<Hexa>& helium, std::vector<Hexa>& sbt) {
    for (unsigned int i = 0; i < dv->getNChildVols(); ++i) {
        const GeoVPhysVol* child = &*dv->getChildVol(i);
        const GeoLogVol* lv = child->getLogVol();
        const GeoShape* shape = lv->getShape();

        std::array<Vec, 8> local;
        if (const auto* b = dynamic_cast<const GeoBox*>(shape))
            local = boxVertices(*b);
        else if (const auto* t = dynamic_cast<const GeoTrap*>(shape))
            local = trapVertices(*t);
        else
            continue;  // no other shape types are placed

        const GeoTrf::Transform3D x = dv->getXToChildVol(i);
        Hexa h;
        h.name = lv->getName();
        for (int k = 0; k < 8; ++k)
            h.v[k] = x * local[k];

        (h.name.find("helium") != std::string::npos ? helium : sbt).push_back(h);
    }
}

struct Built {
    GeoPhysVol* dv = nullptr;
    std::vector<Hexa> helium, sbt;
};

// Build via the factory, from the constants in SBTConstants.h.
Built buildDecayVolume() {
    static Materials materials;
    SHiP::geometry::DecayVolumeFactory factory(materials);
    Built b;
    b.dv = factory.build();
    REQUIRE(b.dv != nullptr);
    collect(b.dv, b.helium, b.sbt);
    return b;
}

// Build a perturbed copy of kSBT. The result is still a constant expression,
// which is what keeps the sweep's parameters compile-time values.
constexpr SBT::SBTParams vary(void (*apply)(SBT::SBTParams&)) {
    SBT::SBTParams p = SBT::kSBT;
    apply(p);
    return p;
}

// Build the SBT + helium from arbitrary parameters into a generous throwaway
// container, bypassing DecayVolumeFactory. This is what lets us sweep: the
// real envelope allocation would reject half the variations below, which vary
// the frustum well past it on purpose.
Built buildFromParams(const SBT::SBTParams& params, const std::string& tag) {
    static Materials materials;
    const GeoMaterial* air = materials.requireMaterial(SBT::kAirMaterial);
    const GeoMaterial* steel = materials.requireMaterial(SBT::kSteelMaterial);
    const GeoMaterial* alMat = materials.requireMaterial(SBT::kWallMaterial);
    const GeoMaterial* labMat = materials.requireMaterial(SBT::kScintillatorMaterial);
    const GeoMaterial* helium = materials.requireMaterial(SBT::kHeliumMaterial);

    // Generous container: this test cares about helium-vs-SBT, not the envelope.
    auto* boxShape = new GeoBox(10000.0, 10000.0, 40000.0);
    auto* container = new GeoPhysVol(new GeoLogVol("/SHiP/test_container", boxShape, air));

    SHiP::geometry::SBTStructureBuilder::build(container, steel, tag + "/structure", params);
    SHiP::geometry::SBTSensorBuilder::build(container, alMat, labMat, tag + "/sensors", params);

    // Derived here rather than read from kHeliumPieces, since the whole point
    // is a configuration the shipped array does not describe.
    const SBT::HeliumSlabs slabs = SBT::heliumSlabs(params);
    REQUIRE(slabs.count == 2u * static_cast<std::size_t>(params.nSubFrustum));
    SBT::buildHelium(container, helium, slabs.view());

    Built b;
    b.dv = container;
    collect(container, b.helium, b.sbt);
    return b;
}
// Geometric tolerance for the SAT assertions. Must stay well below
// heliumClearance (1 um), or the clearance checks become vacuous; and well
// above double-precision noise on ~1e4 mm coordinates (~1e-8 mm).
constexpr double kTol = 1e-6;

// heliumClearance is a gap measured along a coordinate axis. SAT returns a
// Euclidean distance, and the surfaces bounding the helium are tilted by the
// frustum taper, so an axis gap of c shows up as c*cos(tilt). Assert the band.
//
// A bounding surface can tilt in both x and y at once (a side-container
// tracking-piece inner face does), and its normal then makes an angle
// atan(sqrt(gx^2 + gy^2)) with the axis, not atan(max(gx, gy)). The two-axis
// combination is the rigorous lower bound; max() alone overestimates the gap.
double minExpectedSeparation(const SBT::SBTParams& params = SBT::kSBT) {
    const double gx = SBT::xGrowth(params), gy = SBT::yGrowth(params);
    return params.heliumClearance / std::sqrt(1.0 + gx * gx + gy * gy);
}

// Closest approach between any helium slab and any SBT volume.
// > 0 disjoint, 0 touching, < 0 overlapping.
double closestApproach(const Built& b, std::string* culprit = nullptr) {
    double worst = std::numeric_limits<double>::infinity();
    for (const Hexa& he : b.helium) {
        double zlo = std::numeric_limits<double>::infinity(), zhi = -zlo;
        for (const Vec& p : he.v) {
            zlo = std::min(zlo, p.z());
            zhi = std::max(zhi, p.z());
        }
        for (const Hexa& o : b.sbt) {
            double ozlo = std::numeric_limits<double>::infinity(), ozhi = -ozlo;
            for (const Vec& p : o.v) {
                ozlo = std::min(ozlo, p.z());
                ozhi = std::max(ozhi, p.z());
            }
            if (ozhi < zlo - 1.0 || ozlo > zhi + 1.0)
                continue;  // cheap Z reject; a disjoint pair cannot be the minimum
            const double s = separation(he, o);
            if (s < worst) {
                worst = s;
                if (culprit)
                    *culprit = he.name + " vs " + o.name;
            }
        }
    }
    return worst;
}

}  // namespace

// The whole point. No helium slab may intersect any SBT volume — not the
// scintillator containers, not the columns, not the corner beams, and (the one
// PR #58 missed) not the inner flanges of the top/bottom longitudinal beams,
// which hang below the sensor plane into the decay region.
TEST_CASE("HeliumDoesNotOverlapAnySBTVolume", "[decayvolume][envelope]") {
    const Built b = buildDecayVolume();
    REQUIRE(!b.helium.empty());
    REQUIRE(b.sbt.size() > 100);

    std::string culprit;
    const double worst = closestApproach(b, &culprit);

    INFO("closest approach: " << worst << " mm, between " << culprit);
    CHECK(worst >= -kTol);  // NOLINT(readability/check)
}

// ... and no unphysical margin either: the helium must actually track the
// material that bounds it. If a future change to the SBT introduced a volume
// that SBTEnvelope does not know about, the test above would fail; if
// SBTEnvelope became over-conservative, this one would.
TEST_CASE("HeliumIsFlushWithTheSBT", "[decayvolume][envelope]") {
    const Built b = buildDecayVolume();
    const double worst = closestApproach(b);

    INFO("closest approach: " << worst << " mm; want [" << minExpectedSeparation() << ", "
                              << SBT::kSBT.heliumClearance << "]");
    CHECK(worst >= minExpectedSeparation() - kTol);    // NOLINT(readability/check) no gouging
    CHECK(worst <= SBT::kSBT.heliumClearance + kTol);  // NOLINT(readability/check) no margin
}

// The helium fills the analytic envelope exactly, sampled densely rather than
// only at the slab boundaries — this catches an envelope whose knots are in the
// wrong places (e.g. if zSplitOffset() changed but detail::knotAt did not).
TEST_CASE("HeliumMatchesAnalyticEnvelope", "[decayvolume][envelope]") {
    static_assert(SBT::kHeliumPieces.size() == 2u * SBT::kSBT.nSubFrustum);

    for (const auto& p : SBT::kHeliumPieces) {
        for (int k = 0; k <= 32; ++k) {
            const double t = static_cast<double>(k) / 32.0;
            const double z = p.z_lo_mm + t * (p.z_hi_mm - p.z_lo_mm);
            const double dx = p.dx_lo_mm + t * (p.dx_hi_mm - p.dx_lo_mm);
            const double dy = p.dy_lo_mm + t * (p.dy_hi_mm - p.dy_lo_mm);

            // Sample strictly inside the slab so the flat/tracking branch of
            // the X envelope is evaluated on the right side of a knot.
            const double zs = std::min(std::max(z, p.z_lo_mm + 1e-6), p.z_hi_mm - 1e-6);
            const double freeX = SBT::innerFreeHalfX(zs);
            const double freeY = SBT::innerFreeHalfY(zs);

            // Upper bound: the helium never protrudes past the analytic envelope.
            CHECK(dx <= freeX - SBT::kSBT.heliumClearance + kTol);  // NOLINT(readability/check)
            CHECK(dy <= freeY - SBT::kSBT.heliumClearance + kTol);  // NOLINT(readability/check)

            // Lower bound: the helium is flush, not merely inside. In Y the
            // envelope is continuous across a slab, so the interpolated edge
            // must sit right on it. In X it steps at each flat/tracking knot:
            // envelopeAtKnot freezes the slab's dx at the lower (flat) value,
            // while freeX just inside the tracking piece is up to
            // xGrowth * zSplitOffset higher — so allow exactly that documented
            // sawtooth slack there, and no more.
            const double xSlack = std::abs(SBT::xGrowth()) * SBT::zSplitOffset();
            CHECK(dx >=
                  freeX - SBT::kSBT.heliumClearance - xSlack - kTol);  // NOLINT(readability/check)
            CHECK(dy >= freeY - SBT::kSBT.heliumClearance - kTol);     // NOLINT(readability/check)
        }
    }
}

// The shipped helium slabs, pinned bit for bit. The tests above prove the
// slabs are right; this proves they did not move, which a refactor of the
// derivation must not do. Changing the SBT deliberately means updating the
// value: on a mismatch Catch2 prints the new one in hex.
TEST_CASE("HeliumSlabsAreUnchanged", "[decayvolume][envelope]") {
    std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a over the raw bit patterns
    for (const auto& p : SBT::kHeliumPieces) {
        for (const double d :
             {p.z_lo_mm, p.z_hi_mm, p.dx_lo_mm, p.dx_hi_mm, p.dy_lo_mm, p.dy_hi_mm}) {
            h = (h ^ std::bit_cast<std::uint64_t>(d)) * 0x100000001b3ULL;
        }
    }
    CHECK(h == 0xfffbdca57c0cfc67ULL);  // NOLINT(readability/check)
}

// detail::sqrtConst stands in for std::sqrt, which is not constexpr. For the
// shipped taper it must agree with the correctly rounded result exactly, or
// the longitudinal beams and the helium above them move. The argument is a
// constant so that the comparison cannot be skewed by FMA contraction.
TEST_CASE("LongBeamTaperMatchesStdSqrt", "[decayvolume][envelope]") {
    constexpr double g = SBT::yGrowth();
    constexpr double arg = 1.0 + g * g;
    CHECK(SBT::detail::longBeamTaper() == std::sqrt(arg));  // NOLINT(readability/check)
}

// THE test for "does this survive changes to the SBT?". A single configuration
// proves nothing about that — it only shows the arithmetic is right at one
// point. So: perturb each parameter the SBT is actually likely to be
// re-specified with, rebuild the structure, the sensors AND the helium from
// scratch, and re-run the overlap check. Every case must still come out flush.
//
// If a future change to either builder breaks the envelope's model of it, this
// fails across the board rather than at one lucky configuration. It is what
// caught the sawtooth and the longitudinal-beam inner flange, neither of which
// the single shipped configuration would have shown.
namespace {
struct Variation {
    const char* what;
    SBT::SBTParams params;
};

// NOLINTBEGIN(readability/braces)
constexpr std::array<Variation, 14> kVariations{{
    {"baseline", SBT::kSBT},
    {"steeper X taper", vary([](SBT::SBTParams& p) { p.xHalfExit = 3000.0; })},
    {"steeper Y taper", vary([](SBT::SBTParams& p) { p.yHalfExit = 4500.0; })},
    {"no taper at all", vary([](SBT::SBTParams& p) {
         p.xHalfExit = p.xHalfEntrance;
         p.yHalfExit = p.yHalfEntrance;
     })},
    {"wider flange", vary([](SBT::SBTParams& p) { p.hbeamFlangeWidth = 400.0; })},
    {"taller beam", vary([](SBT::SBTParams& p) { p.hbeamHeight = 400.0; })},
    {"thicker flange", vary([](SBT::SBTParams& p) { p.hbeamFlangeThickness = 30.0; })},
    {"thinner containers", vary([](SBT::SBTParams& p) { p.containerThickness = 120.0; })},
    {"thicker containers", vary([](SBT::SBTParams& p) { p.containerThickness = 300.0; })},
    {"more sub-frusta", vary([](SBT::SBTParams& p) { p.nSubFrustum = 20; })},
    {"fewer sub-frusta", vary([](SBT::SBTParams& p) { p.nSubFrustum = 5; })},
    {"bigger sensor clearance", vary([](SBT::SBTParams& p) { p.sensorClearance = 5.0; })},
    {"non-zero helium clearance", vary([](SBT::SBTParams& p) { p.heliumClearance = 10.0; })},
    {"shorter SBT", vary([](SBT::SBTParams& p) { p.totalLength = 20000.0; })},
}};
// NOLINTEND(readability/braces)
}  // namespace

TEST_CASE("HeliumIsFlushAcrossTheParameterSpace", "[decayvolume][envelope][sweep]") {
    for (std::size_t vi = 0; vi < kVariations.size(); ++vi) {
        const Variation& v = kVariations[vi];
        INFO("variation: " << v.what);
        REQUIRE(SBT::leavesDecayRegion(v.params));

        // Tag by index, not by name: collect() classifies a volume as helium by
        // looking for "helium" in it, and one variation is called "non-zero
        // helium clearance".
        const Built b = buildFromParams(v.params, "/SHiP/sweep_" + std::to_string(vi));
        std::string culprit;
        const double worst = closestApproach(b, &culprit);

        INFO("closest approach " << worst << " mm; want [" << minExpectedSeparation(v.params)
                                 << ", " << v.params.heliumClearance << "], nearest " << culprit);
        CHECK(worst >= -kTol);  // NOLINT(readability/check) no overlap
        CHECK(worst >=
              minExpectedSeparation(v.params) - kTol);    // NOLINT(readability/check) clearance
        CHECK(worst <= v.params.heliumClearance + kTol);  // NOLINT(readability/check) no margin
    }
}

// Guard rail: an SBT whose beams and containers have eaten the whole frustum
// must fail loudly. leavesDecayRegion() says so at compile time, and
// SBTEnvelope.h static_asserts it for the shipped parameters, so none of
// these configurations can become the shipped geometry.
namespace {
constexpr SBT::SBTParams kContainersTooThick =
    vary([](SBT::SBTParams& p) { p.containerThickness = 5000.0; });
// subLength 100 mm < zSplitOffset 131.25 mm, so two knots cross.
constexpr SBT::SBTParams kSubFrustaTooShort = vary([](SBT::SBTParams& p) { p.nSubFrustum = 500; });
constexpr SBT::SBTParams kNegativeClearance =
    vary([](SBT::SBTParams& p) { p.heliumClearance = -5.0; });
// Malformed before any slab is derived: a sensor clearance past half the
// container thickness turns the top/bottom containers inside out ...
constexpr SBT::SBTParams kInsideOutContainers =
    vary([](SBT::SBTParams& p) { p.sensorClearance = 200.0; });
// ... and two flanges thicker than the beam is high leave a negative web.
constexpr SBT::SBTParams kNegativeWeb =
    vary([](SBT::SBTParams& p) { p.hbeamFlangeThickness = 130.0; });

// The assertions proper. A positive control keeps the predicate from passing
// vacuously.
static_assert(SBT::leavesDecayRegion(SBT::kSBT));
static_assert(!SBT::leavesDecayRegion(kContainersTooThick));
static_assert(!SBT::leavesDecayRegion(kSubFrustaTooShort));
static_assert(!SBT::leavesDecayRegion(kNegativeClearance));
static_assert(!SBT::leavesDecayRegion(kInsideOutContainers));
static_assert(!SBT::leavesDecayRegion(kNegativeWeb));
}  // namespace

// The static_asserts above are the test; this reports them per case so a
// regression names the configuration rather than only failing to compile.
//
// Stated plainly so nobody mistakes it for more: this proves
// leavesDecayRegion() discriminates, not that a build with these parameters
// actually fails. A compilation that succeeds cannot assert that another one
// fails; that needs a try_compile/WILL_FAIL harness. What ties the two
// together is that SBTEnvelope.h's shipped static_assert is written in terms
// of this very predicate.
TEST_CASE("HeliumRejectsAnImpossibleSBT", "[decayvolume][envelope]") {
    SECTION("containers larger than the frustum") {
        CHECK(!SBT::leavesDecayRegion(kContainersTooThick));  // NOLINT(readability/check)
    }
    SECTION("sub-frustum shorter than the sensor flat piece") {
        CHECK(!SBT::leavesDecayRegion(kSubFrustaTooShort));  // NOLINT(readability/check)
    }
    SECTION("negative clearance would overlap by construction") {
        CHECK(!SBT::leavesDecayRegion(kNegativeClearance));  // NOLINT(readability/check)
    }
    SECTION("sensor clearance turns the top/bottom containers inside out") {
        CHECK(!SBT::leavesDecayRegion(kInsideOutContainers));  // NOLINT(readability/check)
    }
    SECTION("flanges thicker than the beam leave a negative web") {
        CHECK(!SBT::leavesDecayRegion(kNegativeWeb));  // NOLINT(readability/check)
    }
}

// The builders take arbitrary SBTParams, so they check them too: malformed
// parameters must throw rather than produce negative-size shapes.
TEST_CASE("SBTBuildersRejectMalformedParameters", "[decayvolume][envelope]") {
    static Materials materials;
    const GeoMaterial* air = materials.requireMaterial(SBT::kAirMaterial);
    auto* container = new GeoPhysVol(
        new GeoLogVol("/SHiP/test_container", new GeoBox(10000.0, 10000.0, 40000.0), air));

    for (const SBT::SBTParams& params : {kInsideOutContainers, kNegativeWeb}) {
        CHECK_THROWS_AS(
            SHiP::geometry::SBTStructureBuilder::build(
                container, materials.requireMaterial(SBT::kSteelMaterial), "/SHiP/bad", params),
            std::invalid_argument);
        CHECK_THROWS_AS(
            SHiP::geometry::SBTSensorBuilder::build(
                container, materials.requireMaterial(SBT::kWallMaterial),
                materials.requireMaterial(SBT::kScintillatorMaterial), "/SHiP/bad", params),
            std::invalid_argument);
    }
    // Sensor-only: the wall stack must fit both Z-split container pieces.
    // 7 walls x 20 mm = 140 mm overfills the 131.25 mm near piece; with
    // kSubFrustaTooShort the far piece is negative.
    constexpr SBT::SBTParams kWallsOverfillNearPiece =
        vary([](SBT::SBTParams& p) { p.cellWallThickness = 20.0; });
    for (const SBT::SBTParams& params : {kWallsOverfillNearPiece, kSubFrustaTooShort}) {
        CHECK_THROWS_AS(
            SHiP::geometry::SBTSensorBuilder::build(
                container, materials.requireMaterial(SBT::kWallMaterial),
                materials.requireMaterial(SBT::kScintillatorMaterial), "/SHiP/bad", params),
            std::invalid_argument);
    }
    CHECK(container->getNChildVols() == 0u);  // NOLINT(readability/check)
}
