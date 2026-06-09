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

using namespace GeoModelKernelUnits;

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
    auto* box = new GeoBox(halfX, halfY, halfZ);
    auto* log = new GeoLogVol(name + "_env", box, air);
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
    const int nX = static_cast<int>(std::floor(2.0 * envHalfX / pitch + 1e-6));
    const int nY = static_cast<int>(std::floor(2.0 * envHalfY / pitch + 1e-6));

    const double x0 = -0.5 * (nX - 1) * pitch;
    const double y0 = -0.5 * (nY - 1) * pitch;

    int tileId = 0;
    for (int ix = 0; ix < nX; ++ix) {
        for (int iy = 0; iy < nY; ++iy) {
            const std::string tileName =
                regionName + "/tile_" + std::to_string(ix) + "_" + std::to_string(iy);
            env->add(new GeoNameTag(tileName));
            env->add(new GeoIdentifierTag(tileId++));
            env->add(new GeoTransform(GeoTrf::Translate3D(x0 + ix * pitch, y0 + iy * pitch, 0.0)));
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
    auto* containerBox = new GeoBox(s_halfX * mm, s_halfY * mm, s_halfZ * mm);
    auto* containerLog = new GeoLogVol("/SHiP/upstream_tagger", containerBox, air);
    auto* containerPhys = new GeoFullPhysVol(containerLog);

    // Tile shapes (full thickness s_tileThickness; faces s_fineFace / s_coarseFace).
    const double tileHalfZ = 0.5 * s_tileThickness * mm;
    const double finePitch = s_fineFace * mm;
    const double coarsePitch = s_coarseFace * mm;
    const double envHalfZ = tileHalfZ + s_env_z_margin * mm;

    // One reusable GeoLogVol per granularity, shared across all regions
    // (the GeoModel idiom used by the calorimeter bar layers).
    auto* fineTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/fine_tile",
        new GeoBox(0.5 * s_fineFace * mm, 0.5 * s_fineFace * mm, tileHalfZ), polystyrene);
    auto* coarseTileLog = new GeoLogVol(
        "/SHiP/upstream_tagger/coarse_tile",
        new GeoBox(0.5 * s_coarseFace * mm, 0.5 * s_coarseFace * mm, tileHalfZ), polystyrene);

    // ── Region table (centre / half-extents in mm; coplanar at local z = 0) ──
    // Footprint: X ∈ [-1800,+1800], Y ∈ [-1500,+1500] — fits inside the
    // container half-extents (2200 × 3200) with clearance.
    struct Region {
        const char* name;
        double halfX, halfY, ctrX, ctrY;
        bool fine;
    };
    const Region regions[] = {
        // Central strip, y ∈ [-200,+200]
        {"/SHiP/upstream_tagger/fine_left", 200.0, 200.0, -800.0, 0.0, true},
        {"/SHiP/upstream_tagger/fine_right", 200.0, 200.0, +800.0, 0.0, true},
        {"/SHiP/upstream_tagger/coarse_central", 600.0, 200.0, 0.0, 0.0, false},
        // Outer coarse bands
        {"/SHiP/upstream_tagger/coarse_top", 1000.0, 650.0, 0.0, +850.0, false},
        {"/SHiP/upstream_tagger/coarse_bottom", 1000.0, 650.0, 0.0, -850.0, false},
        // Full-height fine extensions (±80 cm in X)
        {"/SHiP/upstream_tagger/ext_left", 400.0, 1500.0, -1400.0, 0.0, true},
        {"/SHiP/upstream_tagger/ext_right", 400.0, 1500.0, +1400.0, 0.0, true},
    };

    int regionId = 0;
    for (const auto& r : regions) {
        const double halfX = r.halfX * mm;
        const double halfY = r.halfY * mm;
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
