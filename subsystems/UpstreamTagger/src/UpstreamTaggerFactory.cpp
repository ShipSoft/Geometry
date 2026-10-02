// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "UpstreamTagger/SHiPUBTManager.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

#include <cmath>
#include <string>

namespace SHiPGeometry {

using GeoModelKernelUnits::mm;

// ── file-scope helpers ───────────────────────────────────────────────────────
namespace {

// Small navigator margin so tiles never touch the region-envelope faces in Z
// (coincident faces trigger GeomNav stuck-track warnings).
constexpr double s_env_z_margin = 0.1;  // mm

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

// Fill @p env with a regular grid of identical tiles sharing @p tileLog.
// The tile count is the number that fully fits (floor), centred symmetrically
// so any transverse remainder becomes equal air margins at the edges — this
// keeps every tile strictly inside the envelope (no overflow / overlap).
void placeTileGrid(GeoVPhysVol* env, const GeoLogVol* tileLog, const std::string& regionName,
                   double envHalfX, double envHalfY, double pitch) {
    const int nX = static_cast<int>(std::floor((2.0 * envHalfX / pitch) + 1e-6));
    const int nY = static_cast<int>(std::floor((2.0 * envHalfY / pitch) + 1e-6));

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
    auto const* containerBox = new GeoBox(s_halfX * mm, s_halfY * mm, s_halfZ * mm);
    auto const* containerLog = new GeoLogVol("/SHiP/upstream_tagger", containerBox, air);
    auto* containerPhys = new GeoFullPhysVol(containerLog);

    // Tile shapes (faces s_fineFace / s_coarseFace; full thicknesses
    // s_fineThickness / s_coarseThickness).
    const double fineHalfZ = 0.5 * s_fineThickness * mm;
    const double coarseHalfZ = 0.5 * s_coarseThickness * mm;
    const double finePitch = s_fineFace * mm;
    const double coarsePitch = s_coarseFace * mm;

    // One reusable GeoLogVol per granularity, shared across all regions
    // (the GeoModel idiom used by the calorimeter bar layers).
    auto const* fineTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/fine_tile",
        new GeoBox(0.5 * s_fineFace * mm, 0.5 * s_fineFace * mm, fineHalfZ), polystyrene);
    auto const* coarseTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/coarse_tile",
        new GeoBox(0.5 * s_coarseFace * mm, 0.5 * s_coarseFace * mm, coarseHalfZ), polystyrene);

    // ── Region table (centre / half-extents in mm; coplanar at local z = 0) ──
    // Footprint: X ∈ [-2200,+2200], Y ∈ [-3200,+3200] — the tiles cover the
    // full container cross-section (4.4 × 6.4 m).
    struct Region {
        const char* name;
        double halfX, halfY, ctrX, ctrY;
        bool fine;
    };
    const Region regions[] = {
        // Central strip, y ∈ [-200,+200]
        {.name = "/SHiP/upstream_tagger/fine_left",
         .halfX = 200.0,
         .halfY = 200.0,
         .ctrX = -800.0,
         .ctrY = 0.0,
         .fine = true},
        {.name = "/SHiP/upstream_tagger/fine_right",
         .halfX = 200.0,
         .halfY = 200.0,
         .ctrX = +800.0,
         .ctrY = 0.0,
         .fine = true},
        {.name = "/SHiP/upstream_tagger/coarse_central",
         .halfX = 600.0,
         .halfY = 200.0,
         .ctrX = 0.0,
         .ctrY = 0.0,
         .fine = false},
        // Outer coarse bands
        {.name = "/SHiP/upstream_tagger/coarse_top",
         .halfX = 1000.0,
         .halfY = 1500.0,
         .ctrX = 0.0,
         .ctrY = +1700.0,
         .fine = false},
        {.name = "/SHiP/upstream_tagger/coarse_bottom",
         .halfX = 1000.0,
         .halfY = 1500.0,
         .ctrX = 0.0,
         .ctrY = -1700.0,
         .fine = false},
        // Full-height fine extensions, |x| from 1.0 m to the container edge
        {.name = "/SHiP/upstream_tagger/ext_left",
         .halfX = 600.0,
         .halfY = 3200.0,
         .ctrX = -1600.0,
         .ctrY = 0.0,
         .fine = true},
        {.name = "/SHiP/upstream_tagger/ext_right",
         .halfX = 600.0,
         .halfY = 3200.0,
         .ctrX = +1600.0,
         .ctrY = 0.0,
         .fine = true},
    };

    int regionId = 0;
    for (const auto& r : regions) {
        const double halfX = r.halfX * mm;
        const double halfY = r.halfY * mm;
        const double envHalfZ = (r.fine ? fineHalfZ : coarseHalfZ) + (s_env_z_margin * mm);
        auto* env = makeRegionEnvelope(containerPhys, air, r.name, regionId++, halfX, halfY,
                                       envHalfZ, r.ctrX * mm, r.ctrY * mm, 0.0);
        placeTileGrid(env, r.fine ? fineTileLog : coarseTileLog, r.name, halfX, halfY,
                      r.fine ? finePitch : coarsePitch);
    }

    if (manager) {
        manager->setContainerVolume(containerPhys);
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
