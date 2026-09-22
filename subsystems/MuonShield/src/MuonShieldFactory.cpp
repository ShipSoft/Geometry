// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "MuonShield/MuonShieldFactory.h"

#include "MuonShield/MuonShieldConfig.h"
#include "SHiPGeometry/ConfigPath.h"
#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoShapeShift.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrd.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <string>

// Absolute fallback path baked in by CMake so out-of-source builds always find
// muon_shield.toml even when the CWD doesn't contain a copy of it.
#ifndef MS_TOML_DEFAULT_PATH
#define MS_TOML_DEFAULT_PATH "muon_shield.toml"
#endif
// Install-time data directory path, set by CMake during install configuration.
#ifndef MS_TOML_INSTALL_PATH
#define MS_TOML_INSTALL_PATH ""
#endif

namespace SHiPGeometry {

namespace {

// Extrinsic X→Y→Z rotation, the convention shared by blocks, reservations and
// embedded daughters.
GeoTrf::Transform3D makeRotation(const std::array<double, 3>& deg) {
    return GeoTrf::RotateZ3D(deg[2] * kDegToRad) * GeoTrf::RotateY3D(deg[1] * kDegToRad) *
           GeoTrf::RotateX3D(deg[0] * kDegToRad);
}

// The 8 corners of a box with the given half-extents, transformed by @p trf.
std::array<GeoTrf::Vector3D, 8> corners(const GeoTrf::Transform3D& trf, double hx, double hy,
                                        double hz) {
    std::array<GeoTrf::Vector3D, 8> out{};
    std::size_t i = 0;
    for (const double sx : {-1.0, 1.0})
        for (const double sy : {-1.0, 1.0})
            for (const double sz : {-1.0, 1.0})
                out[i++] = trf * GeoTrf::Vector3D(sx * hx, sy * hy, sz * hz);
    return out;
}

// Axis-aligned bounds of a corner set: {loX, hiX, loY, hiY, loZ, hiZ}.
template <std::size_t N>
std::array<double, 6> boundsOf(const std::array<GeoTrf::Vector3D, N>& pts) {
    std::array<double, 6> b{1e30, -1e30, 1e30, -1e30, 1e30, -1e30};
    for (const auto& p : pts) {
        b[0] = std::min(b[0], p.x());
        b[1] = std::max(b[1], p.x());
        b[2] = std::min(b[2], p.y());
        b[3] = std::max(b[3], p.y());
        b[4] = std::min(b[4], p.z());
        b[5] = std::max(b[5], p.z());
    }
    return b;
}

}  // namespace

// ── constructor / accessors ──────────────────────────────────────────────────

MuonShieldFactory::MuonShieldFactory(SHiPMaterials& materials, std::string configPath)
    : m_materials(materials), m_configPath(std::move(configPath)) {}

void MuonShieldFactory::embedDaughter(GeoPhysVol* daughter,
                                      const std::array<double, 3>& worldCentre_mm,
                                      const std::array<double, 3>& rotation_deg,
                                      const std::string& name) {
    if (m_built)
        throw std::runtime_error(
            "MuonShieldFactory: embedDaughter() must be called before build()");
    m_daughters.push_back({daughter, worldCentre_mm, rotation_deg, name});
}

void MuonShieldFactory::reserveSpace(const std::array<double, 3>& worldCentre_mm,
                                     const std::array<double, 3>& size_mm,
                                     const std::array<double, 3>& rotation_deg) {
    if (m_built)
        throw std::runtime_error("MuonShieldFactory: reserveSpace() must be called before build()");
    m_reservations.push_back({worldCentre_mm, size_mm, rotation_deg});
}

std::string MuonShieldFactory::resolvedConfigPath() const {
    return resolveConfigPath(m_configPath, MS_TOML_DEFAULT_PATH, MS_TOML_INSTALL_PATH);
}

double MuonShieldFactory::centreZ_mm() const {
    if (!m_built)
        throw std::runtime_error(
            "MuonShieldFactory: centreZ_mm() is only valid after a successful build()");
    return m_centreZ_mm;
}

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* MuonShieldFactory::build() {
    if (m_built)
        throw std::runtime_error("MuonShieldFactory: build() has already been called");

    const MuonShieldConfig cfg = readMuonShieldConfig(resolvedConfigPath());

    GeoMaterial* air = m_materials.requireMaterial("Air");
    GeoMaterial* absorber = m_materials.requireMaterial(cfg.block_material);

    // Envelope-derived dimensions (mm).
    const double halfZ = 0.5 * cfg.envelopeLengthZ_mm();
    m_centreZ_mm = cfg.envelopeCentreZ_mm();

    // Air container spanning the full muon-shield envelope, centred on its own
    // origin (SHiPGeometryBuilder places it at centreZ_mm()).
    auto* containerBox = new GeoBox(cfg.envelope_half_x_mm, cfg.envelope_half_y_mm, halfZ);
    auto* containerLog = new GeoLogVol("/SHiP/muon_shield", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Place each iron block. A block is anchored by its upstream (−z) face at
    // its `start`, extends downstream by size[2], is rotated about `start`
    // (extrinsic X→Y→Z), and may taper symmetrically into a GeoTrd.
    int childId = 0;
    for (const auto& b : cfg.blocks) {
        const double halfX = 0.5 * b.size[0];
        const double halfY = 0.5 * b.size[1];
        const double halfLenZ = 0.5 * b.size[2];

        const double farHalfX = halfX + b.size[2] * std::tan(b.taper_deg[0] * kDegToRad);
        const double farHalfY = halfY + b.size[2] * std::tan(b.taper_deg[1] * kDegToRad);

        // Placement transform: shift the solid (centred at its origin) so its
        // upstream face sits at `start`, rotate about `start` (extrinsic
        // X→Y→Z), then translate into place. Container is at m_centreZ_mm in Z.
        const double anchorX = b.start[0];
        const double anchorY = b.start[1];
        const double anchorZ = b.start[2] - m_centreZ_mm;
        const GeoTrf::Transform3D rotation = makeRotation(b.rotation_deg);
        const GeoTrf::Transform3D trf = GeoTrf::Translate3D(anchorX, anchorY, anchorZ) * rotation *
                                        GeoTrf::Translate3D(0.0, 0.0, halfLenZ);

        GeoShape* shape = nullptr;
        if (b.taper_deg[0] == 0.0 && b.taper_deg[1] == 0.0)
            shape = new GeoBox(halfX, halfY, halfLenZ);
        else
            shape = new GeoTrd(halfX, farHalfX, halfY, farHalfY, halfLenZ);

        // Block AABB in the container frame, from its 8 transformed corners
        // (exact under rotation). The upstream corners use the near-face
        // half-widths and the downstream ones the far-face half-widths, so a
        // tapered block is bounded exactly rather than as a full-width box —
        // otherwise a reservation that only grazes the wide end would carve
        // magnets it never touches.
        std::array<GeoTrf::Vector3D, 8> blockCorners{};
        std::size_t ci = 0;
        for (const double sz : {-1.0, 1.0}) {
            const double faceHX = sz < 0.0 ? halfX : farHalfX;
            const double faceHY = sz < 0.0 ? halfY : farHalfY;
            for (const double sx : {-1.0, 1.0})
                for (const double sy : {-1.0, 1.0})
                    blockCorners[ci++] =
                        trf * GeoTrf::Vector3D(sx * faceHX, sy * faceHY, sz * halfLenZ);
        }
        const auto bb = boundsOf(blockCorners);
        const double bLoX = bb[0], bHiX = bb[1], bLoY = bb[2], bHiY = bb[3], bLoZ = bb[4],
                     bHiZ = bb[5];

        // Reject any block whose bounding box pokes out of the Air container.
        constexpr double kEps = 1e-6;
        if (bLoX < -cfg.envelope_half_x_mm - kEps || bHiX > cfg.envelope_half_x_mm + kEps ||
            bLoY < -cfg.envelope_half_y_mm - kEps || bHiY > cfg.envelope_half_y_mm + kEps ||
            bLoZ < -halfZ - kEps || bHiZ > halfZ + kEps)
            throw std::runtime_error("MuonShieldFactory: block " + std::to_string(childId) +
                                     " bounding box exceeds the Air container");

        // Carve out any reserved box that intersects this magnet (A - B). Both
        // the block AABB (above) and the reservation AABB (below) account for
        // rotation; the subtracted box is expressed in the block's own frame.
        for (const auto& r : m_reservations) {
            const double rcx = r.centre_mm[0];
            const double rcy = r.centre_mm[1];
            const double rcz = r.centre_mm[2] - m_centreZ_mm;
            const double rhx = 0.5 * r.size_mm[0];
            const double rhy = 0.5 * r.size_mm[1];
            const double rhz = 0.5 * r.size_mm[2];
            const GeoTrf::Transform3D resRot = makeRotation(r.rotation_deg);
            const auto rb =
                boundsOf(corners(GeoTrf::Translate3D(rcx, rcy, rcz) * resRot, rhx, rhy, rhz));
            const double rLoX = rb[0], rHiX = rb[1], rLoY = rb[2], rHiY = rb[3], rLoZ = rb[4],
                         rHiZ = rb[5];
            const bool intersects = bLoX < rHiX && bHiX > rLoX && bLoY < rHiY && bHiY > rLoY &&
                                    bLoZ < rHiZ && bHiZ > rLoZ;
            if (!intersects)
                continue;
            const GeoTrf::Transform3D resInBlock =
                trf.inverse() * GeoTrf::Translate3D(rcx, rcy, rcz) * resRot;
            auto* voidBox = new GeoBox(rhx, rhy, rhz);
            shape = new GeoShapeSubtraction(shape, new GeoShapeShift(voidBox, resInBlock));
        }

        const std::string blockName = "/SHiP/muon_shield/block_" + std::to_string(childId);
        auto* blockLog = new GeoLogVol(blockName, shape, absorber);
        auto* blockPhys = new GeoPhysVol(blockLog);

        containerPhys->add(new GeoNameTag(blockName));
        containerPhys->add(new GeoIdentifierTag(childId++));
        containerPhys->add(new GeoTransform(trf));
        containerPhys->add(blockPhys);
    }

    // Place the embedded daughters inside the container, keeping them as
    // independent, named subsystems.
    //
    // A daughter is checked against its own bounding box, not just its centre:
    // it must fit inside the Air container, and it must lie inside one of the
    // reserved cavities. The second check is what keeps the block list and the
    // daughter honest — the iron is defined entirely by the blocks, so anything
    // outside a cavity is sitting in solid iron. It also catches the case where
    // a daughter's own dimensions grow past the reservation declared for it.
    for (const auto& d : m_daughters) {
        const GeoTrf::Transform3D dTrf =
            GeoTrf::Translate3D(d.centre_mm[0], d.centre_mm[1], d.centre_mm[2] - m_centreZ_mm) *
            makeRotation(d.rotation_deg);

        double dxMin = 0.0, dyMin = 0.0, dzMin = 0.0, dxMax = 0.0, dyMax = 0.0, dzMax = 0.0;
        d.volume->getLogVol()->getShape()->extent(dxMin, dyMin, dzMin, dxMax, dyMax, dzMax);

        // The daughter's local bounding box need not be centred on its origin,
        // so fold its centre offset into the transform before taking corners.
        const GeoTrf::Transform3D dBoxTrf =
            dTrf * GeoTrf::Translate3D(0.5 * (dxMin + dxMax), 0.5 * (dyMin + dyMax),
                                       0.5 * (dzMin + dzMax));
        const double dhx = 0.5 * (dxMax - dxMin);
        const double dhy = 0.5 * (dyMax - dyMin);
        const double dhz = 0.5 * (dzMax - dzMin);
        const auto dCorners = corners(dBoxTrf, dhx, dhy, dhz);

        constexpr double kEps = 1e-6;
        const auto db = boundsOf(dCorners);
        if (db[0] < -cfg.envelope_half_x_mm - kEps || db[1] > cfg.envelope_half_x_mm + kEps ||
            db[2] < -cfg.envelope_half_y_mm - kEps || db[3] > cfg.envelope_half_y_mm + kEps ||
            db[4] < -halfZ - kEps || db[5] > halfZ + kEps)
            throw std::runtime_error(
                "MuonShieldFactory: embedded daughter '" + d.name +
                "' does not fit inside the shield envelope: its bounding box spans x [" +
                std::to_string(db[0]) + ", " + std::to_string(db[1]) + "], y [" +
                std::to_string(db[2]) + ", " + std::to_string(db[3]) + "], z [" +
                std::to_string(db[4]) + ", " + std::to_string(db[5]) +
                "] mm in the container frame, whose half-sizes are " +
                std::to_string(cfg.envelope_half_x_mm) + " x " +
                std::to_string(cfg.envelope_half_y_mm) + " x " + std::to_string(halfZ) + " mm");

        // Containment in a reservation is tested in that reservation's own
        // frame rather than by comparing bounding boxes, so it stays exact when
        // either the daughter or the reservation is rotated.
        const bool inCavity = std::ranges::any_of(m_reservations, [&](const ReservedBox& r) {
            const GeoTrf::Transform3D toRes = (GeoTrf::Translate3D(r.centre_mm[0], r.centre_mm[1],
                                                                   r.centre_mm[2] - m_centreZ_mm) *
                                               makeRotation(r.rotation_deg))
                                                  .inverse();
            return std::ranges::all_of(dCorners, [&](const GeoTrf::Vector3D& c) {
                const GeoTrf::Vector3D p = toRes * c;
                return std::abs(p.x()) <= 0.5 * r.size_mm[0] + kEps &&
                       std::abs(p.y()) <= 0.5 * r.size_mm[1] + kEps &&
                       std::abs(p.z()) <= 0.5 * r.size_mm[2] + kEps;
            });
        });
        if (!inCavity)
            throw std::runtime_error(
                "MuonShieldFactory: embedded daughter '" + d.name +
                "' is not contained in any reserved cavity, so it would intersect shield iron. "
                "Its bounding box is " +
                std::to_string(2.0 * dhx) + " x " + std::to_string(2.0 * dhy) + " x " +
                std::to_string(2.0 * dhz) +
                " mm; call reserveSpace() with a box at least that large at the same centre and "
                "rotation");

        containerPhys->add(new GeoNameTag(d.name));
        containerPhys->add(new GeoIdentifierTag(childId++));
        containerPhys->add(new GeoTransform(dTrf));
        containerPhys->add(d.volume);
    }

    m_built = true;
    return containerPhys;
}

}  // namespace SHiPGeometry
