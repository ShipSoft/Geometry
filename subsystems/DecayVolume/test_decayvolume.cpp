// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/DecayVolumeFactory.h"
#include "DecayVolume/SBTConfig.h"
#include "DecayVolume/SBTEnvelope.h"
#include "DecayVolume/SBTSensorBuilder.h"
#include "DecayVolume/SBTStructureBuilder.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <vector>

using SHiPGeometry::SHiPMaterials;

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
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
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
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
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
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    const ChildShapeCounts c = countByShape(dv);
    CHECK(c.traps == 3400u);  // NOLINT(readability/check)
    CHECK(c.helium == 20u);   // NOLINT(readability/check)
}

// Flat architecture: total direct children = 312 structure + 3380 sensors +
// 20 helium slabs = 3712, with no grandchildren.
TEST_CASE("DecayVolumeChildCount", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    GeoPhysVol* dv = factory.build();
    REQUIRE(dv != nullptr);
    CHECK(dv->getNChildVols() == 3712u);  // NOLINT(readability/check)
}

// The central decay region is built from helium GeoTraps, derived from the
// innermost SBT surfaces so that they cannot overlap the structure or sensors.
TEST_CASE("DecayVolumeHasHeliumFrustum", "[decayvolume]") {
    SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
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

std::array<Vec, 8> trapVertices(const GeoTrap& t) {
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
    SHiPGeometry::SBTConfig cfg;  // the config the geometry was ACTUALLY built from
    std::vector<Hexa> helium, sbt;
};

// Build via the factory, i.e. from sbt.toml. Carries the resolved config back
// out, so the tests never compare the built geometry against a default-
// constructed SBTConfig that may say something different.
Built buildFromToml() {
    static SHiPMaterials materials;
    SHiPGeometry::DecayVolumeFactory factory(materials);
    Built b;
    b.dv = factory.build();
    REQUIRE(b.dv != nullptr);
    b.cfg = factory.config();
    collect(b.dv, b.helium, b.sbt);
    return b;
}

// Build the SBT + helium directly from an arbitrary SBTConfig, bypassing the
// toml. This is what lets us sweep the parameter space.
Built buildFromConfig(const SHiPGeometry::SBTConfig& cfg) {
    static SHiPMaterials materials;
    const GeoMaterial* air = materials.requireMaterial(cfg.material_air);
    const GeoMaterial* steel = materials.requireMaterial(cfg.material_steel);
    const GeoMaterial* alMat = materials.requireMaterial(cfg.material_wall);
    const GeoMaterial* labMat = materials.requireMaterial(cfg.material_cell);
    const GeoMaterial* helium = materials.requireMaterial(cfg.material_helium);

    // Generous container: this test cares about helium-vs-SBT, not the envelope.
    auto* boxShape = new GeoBox(10000.0, 10000.0, 40000.0);
    auto* boxLog = new GeoLogVol("/SHiP/test_container", boxShape, air);
    auto* container = new GeoPhysVol(boxLog);

    SHiPGeometry::SBTStructureBuilder::build(container, steel, cfg);
    SHiPGeometry::SBTSensorBuilder::build(container, alMat, labMat, cfg);
    SHiPGeometry::buildHelium(container, helium, cfg);

    Built b;
    b.dv = container;
    b.cfg = cfg;
    collect(container, b.helium, b.sbt);
    return b;
}

// Geometric tolerance for the SAT assertions. Must stay well below
// helium_clearance_mm (1 um), or the clearance checks become vacuous; and well
// above double-precision noise on ~1e4 mm coordinates (~1e-8 mm).
constexpr double kTol = 1e-6;

// helium_clearance_mm is a gap measured along a coordinate axis. SAT returns a
// Euclidean distance, and the surfaces bounding the helium are tilted by the
// frustum taper, so an axis gap of c shows up as c*cos(tilt). Assert the band.
double minExpectedSeparation(const SHiPGeometry::SBTConfig& cfg) {
    const double g = std::max(std::abs(cfg.xGrowth()), std::abs(cfg.yGrowth()));
    return cfg.helium_clearance_mm / std::sqrt(1.0 + g * g);
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
    const Built b = buildFromToml();
    REQUIRE(!b.helium.empty());
    REQUIRE(b.sbt.size() > 100);

    std::string culprit;
    const double worst = closestApproach(b, &culprit);

    INFO("closest approach: " << worst << " mm, between " << culprit);
    CHECK(worst >= -kTol);  // NOLINT(readability/check)
}

// ... and no unphysical margin either: with helium_clearance_mm = 0 the helium
// must actually touch the material that bounds it. If a future change to the
// SBT introduced a volume that SBTEnvelope does not know about, the test above
// would fail; if SBTEnvelope became over-conservative, this one would.
TEST_CASE("HeliumIsFlushWithTheSBT", "[decayvolume][envelope]") {
    const Built b = buildFromToml();
    const double worst = closestApproach(b);

    INFO("closest approach: " << worst << " mm; want [" << minExpectedSeparation(b.cfg) << ", "
                              << b.cfg.helium_clearance_mm << "]");
    CHECK(worst >= minExpectedSeparation(b.cfg) - kTol);  // NOLINT(readability/check) no gouging
    CHECK(worst <= b.cfg.helium_clearance_mm + kTol);     // NOLINT(readability/check) no margin
}

// The helium fills the analytic envelope exactly, sampled densely rather than
// only at the slab boundaries — this catches an envelope whose knots are in the
// wrong places (e.g. if zSplitOffset() changed but envelopeKnots() did not).
TEST_CASE("HeliumMatchesAnalyticEnvelope", "[decayvolume][envelope]") {
    const SHiPGeometry::SBTConfig cfg = buildFromToml().cfg;
    const auto pieces = SHiPGeometry::heliumPieces(cfg);
    REQUIRE(pieces.size() == 2u * static_cast<std::size_t>(cfg.n_sub_frustum));

    for (const auto& p : pieces) {
        for (int k = 0; k <= 32; ++k) {
            const double t = static_cast<double>(k) / 32.0;
            const double z = p.z_lo_mm + t * (p.z_hi_mm - p.z_lo_mm);
            const double dx = p.dx_lo_mm + t * (p.dx_hi_mm - p.dx_lo_mm);
            const double dy = p.dy_lo_mm + t * (p.dy_hi_mm - p.dy_lo_mm);

            // Sample strictly inside the slab so the flat/tracking branch of
            // the X envelope is evaluated on the right side of a knot.
            const double zs = std::min(std::max(z, p.z_lo_mm + 1e-6), p.z_hi_mm - 1e-6);
            const double freeX = SHiPGeometry::innerFreeHalfX(cfg, zs);
            const double freeY = SHiPGeometry::innerFreeHalfY(cfg, zs);

            CHECK(dx <= freeX - cfg.helium_clearance_mm + kTol);  // NOLINT(readability/check)
            CHECK(dy <= freeY - cfg.helium_clearance_mm + kTol);  // NOLINT(readability/check)
        }
    }
}

// THE test for "does this survive changes to the SBT?". A single configuration
// proves nothing about that — it only shows the arithmetic is right at one
// point. So: perturb each parameter the SBT is actually likely to be
// re-specified with, rebuild the structure, the sensors AND the helium from
// scratch, and re-run the overlap check. Every case must still come out flush.
//
// If a future change to either builder breaks the envelope's model of it, this
// fails across the board rather than at one lucky configuration.
TEST_CASE("HeliumIsFlushAcrossTheParameterSpace", "[decayvolume][envelope][sweep]") {
    const SHiPGeometry::SBTConfig base = buildFromToml().cfg;

    struct Variation {
        const char* what;
        std::function<void(SHiPGeometry::SBTConfig&)> apply;
    };

    const auto variations = std::vector<Variation>{
        {"baseline", [](SHiPGeometry::SBTConfig&) {}},
        {"steeper X taper", [](SHiPGeometry::SBTConfig& c) { c.x_half_exit_mm = 3000.0; }},
        {"steeper Y taper", [](SHiPGeometry::SBTConfig& c) { c.y_half_exit_mm = 4500.0; }},
        {"no taper at all",
         [](SHiPGeometry::SBTConfig& c) {
             c.x_half_exit_mm = c.x_half_entrance_mm;
             c.y_half_exit_mm = c.y_half_entrance_mm;
         }},
        {"wider flange", [](SHiPGeometry::SBTConfig& c) { c.hbeam_flange_width_mm = 400.0; }},
        {"taller beam", [](SHiPGeometry::SBTConfig& c) { c.hbeam_height_mm = 400.0; }},
        {"thicker flange", [](SHiPGeometry::SBTConfig& c) { c.hbeam_flange_thickness_mm = 30.0; }},
        {"thinner containers",
         [](SHiPGeometry::SBTConfig& c) { c.container_thickness_mm = 120.0; }},
        {"thicker containers",
         [](SHiPGeometry::SBTConfig& c) { c.container_thickness_mm = 300.0; }},
        {"more sub-frusta", [](SHiPGeometry::SBTConfig& c) { c.n_sub_frustum = 20; }},
        {"fewer sub-frusta", [](SHiPGeometry::SBTConfig& c) { c.n_sub_frustum = 5; }},
        {"bigger sensor clearance",
         [](SHiPGeometry::SBTConfig& c) { c.sensor_clearance_mm = 5.0; }},
        {"non-zero helium clearance",
         [](SHiPGeometry::SBTConfig& c) { c.helium_clearance_mm = 10.0; }},
        {"shorter SBT", [](SHiPGeometry::SBTConfig& c) { c.total_length_mm = 20000.0; }},
    };

    for (const Variation& v : variations) {
        SHiPGeometry::SBTConfig cfg = base;
        v.apply(cfg);

        const Built b = buildFromConfig(cfg);
        std::string culprit;
        const double worst = closestApproach(b, &culprit);

        INFO("variation: " << v.what << " -> closest approach " << worst << " mm; want ["
                           << minExpectedSeparation(cfg) << ", " << cfg.helium_clearance_mm
                           << "], nearest " << culprit);
        CHECK(worst >= -kTol);  // NOLINT(readability/check) no overlap
        CHECK(worst >=
              minExpectedSeparation(cfg) - kTol);        // NOLINT(readability/check) clearance kept
        CHECK(worst <= cfg.helium_clearance_mm + kTol);  // NOLINT(readability/check) no margin
    }
}

// Guard rail: an SBT whose beams and containers have eaten the whole frustum
// must fail loudly, not silently produce an inverted GeoTrap.
TEST_CASE("HeliumRejectsAnImpossibleSBT", "[decayvolume][envelope]") {
    SECTION("containers larger than the frustum") {
        SHiPGeometry::SBTConfig cfg;
        cfg.container_thickness_mm = 5000.0;
        CHECK_THROWS(SHiPGeometry::heliumPieces(cfg));
    }
    SECTION("sub-frustum shorter than the sensor flat piece") {
        SHiPGeometry::SBTConfig cfg;
        cfg.n_sub_frustum = 500;  // subLength 100 mm < zSplitOffset 131.25 mm
        CHECK_THROWS(SHiPGeometry::heliumPieces(cfg));
    }
    SECTION("negative clearance would overlap by construction") {
        SHiPGeometry::SBTConfig cfg;
        cfg.helium_clearance_mm = -5.0;
        CHECK_THROWS(SHiPGeometry::heliumPieces(cfg));
    }
}
