// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "SHiPGeometry/StaticChecks.h"
#include "UpstreamTagger/SHiPUBTManager.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>

#include <array>
#include <cstddef>
#include <string>

namespace SHiPGeometry {

using units::gm;
using units::mm;

// ── file-scope helpers ───────────────────────────────────────────────────────
namespace {

// Small navigator margin so tiles never touch the region-envelope faces in Z
// (coincident faces trigger GeomNav stuck-track warnings).
constexpr auto s_env_z_margin = 0.1 * units::mm;

// ── Region table (centres / half-extents; coplanar at local z = 0) ──
// Footprint: X ∈ [-2200,+2200], Y ∈ [-3200,+3200] — the tiles cover the
// full container cross-section (4.4 × 6.4 m).
struct Region {
    const char* name;
    units::LengthMm halfX, halfY, ctrX, ctrY;
    bool fine;

    [[nodiscard]] constexpr units::LengthMm pitch() const {
        return fine ? UpstreamTaggerFactory::s_fineFace : UpstreamTaggerFactory::s_coarseFace;
    }
    [[nodiscard]] constexpr units::LengthMm thickness() const {
        return fine ? UpstreamTaggerFactory::s_fineThickness
                    : UpstreamTaggerFactory::s_coarseThickness;
    }
};
constexpr std::array kRegions = std::to_array<Region>({
    // Central strip, y ∈ [-200,+200]
    {.name = "/SHiP/upstream_tagger/fine_left",
     .halfX = 200.0 * mm,
     .halfY = 200.0 * mm,
     .ctrX = -800.0 * mm,
     .ctrY = 0.0 * mm,
     .fine = true},
    {.name = "/SHiP/upstream_tagger/fine_right",
     .halfX = 200.0 * mm,
     .halfY = 200.0 * mm,
     .ctrX = +800.0 * mm,
     .ctrY = 0.0 * mm,
     .fine = true},
    {.name = "/SHiP/upstream_tagger/coarse_central",
     .halfX = 600.0 * mm,
     .halfY = 200.0 * mm,
     .ctrX = 0.0 * mm,
     .ctrY = 0.0 * mm,
     .fine = false},
    // Outer coarse bands
    {.name = "/SHiP/upstream_tagger/coarse_top",
     .halfX = 1000.0 * mm,
     .halfY = 1500.0 * mm,
     .ctrX = 0.0 * mm,
     .ctrY = +1700.0 * mm,
     .fine = false},
    {.name = "/SHiP/upstream_tagger/coarse_bottom",
     .halfX = 1000.0 * mm,
     .halfY = 1500.0 * mm,
     .ctrX = 0.0 * mm,
     .ctrY = -1700.0 * mm,
     .fine = false},
    // Full-height fine extensions, |x| from 1.0 m to the container edge
    {.name = "/SHiP/upstream_tagger/ext_left",
     .halfX = 600.0 * mm,
     .halfY = 3200.0 * mm,
     .ctrX = -1600.0 * mm,
     .ctrY = 0.0 * mm,
     .fine = true},
    {.name = "/SHiP/upstream_tagger/ext_right",
     .halfX = 600.0 * mm,
     .halfY = 3200.0 * mm,
     .ctrX = +1600.0 * mm,
     .ctrY = 0.0 * mm,
     .fine = true},
});

/// Tiles along one side of a region: the number that fits in 2·half.
constexpr int tileCount(units::LengthMm half, units::LengthMm pitch) {
    return static_cast<int>(units::ratio(2.0 * half / pitch));
}

// ── Compile-time validation ──────────────────────────────────────────────
using UBT = UpstreamTaggerFactory;

constexpr checks::Box regionBox(const Region& r) {
    return checks::box(r.ctrX, r.ctrY, 0.0 * mm, r.halfX, r.halfY,
                       0.5 * r.thickness() + s_env_z_margin);
}

static_assert(checks::firstFailure(kRegions,
                                   [](const Region& r) {
                                       return checks::contains(
                                           checks::box(UBT::s_halfX, UBT::s_halfY, UBT::s_halfZ),
                                           regionBox(r));
                                   }) == checks::npos,
              "a UBT region sticks out of the container");
static_assert(
    [] {
        for (std::size_t i = 0; i < kRegions.size(); ++i) {
            for (std::size_t j = i + 1; j < kRegions.size(); ++j) {
                if (checks::overlaps(regionBox(kRegions[i]), regionBox(kRegions[j]))) {
                    return false;
                }
            }
        }
        return true;
    }(),
    "two UBT regions overlap");
// With no overlaps and every region inside the container, equal areas mean
// the regions cover the whole cross-section.
static_assert(
    [] {
        auto area = 0.0 * mm * mm;
        for (const auto& r : kRegions) {
            area += 4.0 * r.halfX * r.halfY;
        }
        return area == 4.0 * UBT::s_halfX * UBT::s_halfY;
    }(),
    "the UBT regions leave part of the container cross-section uncovered");
// Whole tiles only: then the grid fills each region with no air margin.
static_assert(checks::firstFailure(kRegions,
                                   [](const Region& r) {
                                       return tileCount(r.halfX, r.pitch()) * r.pitch() ==
                                                  2.0 * r.halfX &&
                                              tileCount(r.halfY, r.pitch()) * r.pitch() ==
                                                  2.0 * r.halfY;
                                   }) == checks::npos,
              "a UBT region is not a whole number of tiles wide");
static_assert(
    [] {
        int n = 0;
        for (const auto& r : kRegions) {
            n += tileCount(r.halfX, r.pitch()) * tileCount(r.halfY, r.pitch());
        }
        return n;
    }() == 47000,
    "tile count changed: update the class documentation and test_upstreamtagger");

// Create an air region envelope, place it in @p container at (xc, yc, zc),
// and return it so its tile grid can be added. All inputs in GeoModel units.
GeoPhysVol* makeRegionEnvelope(GeoVPhysVol* container, const GeoMaterial* air,
                               const std::string& name, int id, double halfX, double halfY,
                               double halfZ, double xc, double yc, double zc) {
    auto const* box = new GeoBox(halfX, halfY, halfZ);
    auto const* log = new GeoLogVol(name + "_env", box, air);
    auto* phys = new GeoPhysVol(log);
    container->add(new GeoNameTag(name));
    container->add(new GeoIdentifierTag(id));
    container->add(new GeoTransform(GeoTrf::Translate3D(xc, yc, zc)));
    container->add(phys);
    return phys;
}

// Fill @p env with a regular grid of identical tiles sharing @p tileLog,
// centred on the region. The regions are whole numbers of tiles wide
// (asserted above), so the grid fills the region exactly.
void placeTileGrid(GeoVPhysVol* env, const GeoLogVol* tileLog, const Region& region) {
    const int nX = tileCount(region.halfX, region.pitch());
    const int nY = tileCount(region.halfY, region.pitch());
    const double pitch = gm(region.pitch());
    const std::string regionName = region.name;

    const double x0 = -0.5 * (nX - 1) * pitch;
    const double y0 = -0.5 * (nY - 1) * pitch;

    int tileId = 0;
    for (int ix = 0; ix < nX; ++ix) {
        for (int iy = 0; iy < nY; ++iy) {
            const std::string tileName =
                regionName + "/tile_" + std::to_string(ix) + "_" + std::to_string(iy);
            env->add(new GeoNameTag(tileName));
            env->add(new GeoIdentifierTag(tileId++));
            env->add(
                new GeoTransform(GeoTrf::Translate3D(x0 + (ix * pitch), y0 + (iy * pitch), 0.0)));
            env->add(new GeoPhysVol(tileLog));
        }
    }
}

}  // namespace

// ── constructor ──────────────────────────────────────────────────────────────

UpstreamTaggerFactory::UpstreamTaggerFactory(SHiPMaterials& materials) : m_materials(materials) {}

// ── build ────────────────────────────────────────────────────────────────────

GeoVPhysVol* UpstreamTaggerFactory::build(SHiPUBTManager* manager) {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* polystyrene = m_materials.requireMaterial("Polystyrene");

    // Container: a GeoFullPhysVol so the tagger keeps a sensitive tree-top for
    // SHiPUBTManager. Dimensions are unchanged from the previous slab.
    auto const* containerBox = new GeoBox(gm(s_halfX), gm(s_halfY), gm(s_halfZ));
    auto const* containerLog = new GeoLogVol("/SHiP/upstream_tagger", containerBox, air);
    auto* containerPhys = new GeoFullPhysVol(containerLog);

    // Tile shapes (faces s_fineFace / s_coarseFace; full thicknesses
    // s_fineThickness / s_coarseThickness).
    const double fineHalfZ = 0.5 * gm(s_fineThickness);
    const double coarseHalfZ = 0.5 * gm(s_coarseThickness);

    // One reusable GeoLogVol per granularity, shared across all regions
    // (the GeoModel idiom used by the calorimeter bar layers).
    auto const* fineTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/fine_tile",
        new GeoBox(0.5 * gm(s_fineFace), 0.5 * gm(s_fineFace), fineHalfZ), polystyrene);
    auto const* coarseTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/coarse_tile",
        new GeoBox(0.5 * gm(s_coarseFace), 0.5 * gm(s_coarseFace), coarseHalfZ), polystyrene);

    int regionId = 0;
    for (const auto& r : kRegions) {
        const double halfX = gm(r.halfX);
        const double halfY = gm(r.halfY);
        const double envHalfZ = (r.fine ? fineHalfZ : coarseHalfZ) + gm(s_env_z_margin);
        auto* env = makeRegionEnvelope(containerPhys, air, r.name, regionId++, halfX, halfY,
                                       envHalfZ, gm(r.ctrX), gm(r.ctrY), 0.0);
        placeTileGrid(env, r.fine ? fineTileLog : coarseTileLog, r);
    }

    if (manager) {
        manager->setContainerVolume(containerPhys);
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
