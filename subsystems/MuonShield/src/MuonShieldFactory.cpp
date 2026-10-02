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

#include <algorithm>
#include <array>
#include <cmath>
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

// ── constructor / accessors ──────────────────────────────────────────────────

MuonShieldFactory::MuonShieldFactory(SHiPMaterials& materials, std::string configPath)
    : m_materials(materials), m_configPath(std::move(configPath)) {}

void MuonShieldFactory::embedDaughter(GeoPhysVol* daughter, double worldCentreZ_mm,
                                      const std::string& name) {
    if (m_built)
        throw std::runtime_error(
            "MuonShieldFactory: embedDaughter() must be called before build()");
    m_daughters.push_back({daughter, worldCentreZ_mm, name});
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

// ── build ────────────────────────────────────────────────────────────────────

GeoPhysVol* MuonShieldFactory::build() {
    if (m_built)
        throw std::runtime_error("MuonShieldFactory: build() has already been called");
    m_built = true;

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
        const GeoTrf::Transform3D rotation = GeoTrf::RotateZ3D(b.rotation_deg[2] * kDegToRad) *
                                             GeoTrf::RotateY3D(b.rotation_deg[1] * kDegToRad) *
                                             GeoTrf::RotateX3D(b.rotation_deg[0] * kDegToRad);
        const GeoTrf::Transform3D trf = GeoTrf::Translate3D(anchorX, anchorY, anchorZ) * rotation *
                                        GeoTrf::Translate3D(0.0, 0.0, halfLenZ);

        GeoShape* shape = nullptr;
        if (b.taper_deg[0] == 0.0 && b.taper_deg[1] == 0.0)
            shape = new GeoBox(halfX, halfY, halfLenZ);
        else
            shape = new GeoTrd(halfX, farHalfX, halfY, farHalfY, halfLenZ);

        // Block AABB in the container frame, from its 8 transformed corners
        // (exact under rotation).
        const double blockMaxHX = std::max(halfX, farHalfX);
        const double blockMaxHY = std::max(halfY, farHalfY);
        double bLoX = 1e30, bHiX = -1e30, bLoY = 1e30, bHiY = -1e30, bLoZ = 1e30, bHiZ = -1e30;
        for (const double sx : {-1.0, 1.0})
            for (const double sy : {-1.0, 1.0})
                for (const double sz : {-1.0, 1.0}) {
                    const GeoTrf::Vector3D c =
                        trf * GeoTrf::Vector3D(sx * blockMaxHX, sy * blockMaxHY, sz * halfLenZ);
                    bLoX = std::min(bLoX, c.x());
                    bHiX = std::max(bHiX, c.x());
                    bLoY = std::min(bLoY, c.y());
                    bHiY = std::max(bHiY, c.y());
                    bLoZ = std::min(bLoZ, c.z());
                    bHiZ = std::max(bHiZ, c.z());
                }

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
            const GeoTrf::Transform3D resRot = GeoTrf::RotateZ3D(r.rotation_deg[2] * kDegToRad) *
                                               GeoTrf::RotateY3D(r.rotation_deg[1] * kDegToRad) *
                                               GeoTrf::RotateX3D(r.rotation_deg[0] * kDegToRad);
            double rLoX = 1e30, rHiX = -1e30, rLoY = 1e30, rHiY = -1e30, rLoZ = 1e30, rHiZ = -1e30;
            for (const double sx : {-1.0, 1.0})
                for (const double sy : {-1.0, 1.0})
                    for (const double sz : {-1.0, 1.0}) {
                        const GeoTrf::Vector3D c =
                            resRot * GeoTrf::Vector3D(sx * rhx, sy * rhy, sz * rhz);
                        rLoX = std::min(rLoX, rcx + c.x());
                        rHiX = std::max(rHiX, rcx + c.x());
                        rLoY = std::min(rLoY, rcy + c.y());
                        rHiY = std::max(rHiY, rcy + c.y());
                        rLoZ = std::min(rLoZ, rcz + c.z());
                        rHiZ = std::max(rHiZ, rcz + c.z());
                    }
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
    // independent, named subsystems. (Callers must not list iron blocks over a
    // daughter's footprint — the block list is the sole source of iron.)
    //
    // embedDaughter() takes only a Z centre, so daughters are placed on the beam
    // axis (x = y = 0); the Z fit is therefore the only bound checked here. If
    // off-axis daughters are ever supported, add a transverse-extent check.
    for (const auto& d : m_daughters) {
        const double localZ = d.worldCentreZ_mm - m_centreZ_mm;
        if (std::abs(localZ) > halfZ)
            throw std::runtime_error("MuonShieldFactory: embedded daughter '" + d.name +
                                     "' lies outside the shield envelope in Z");
        containerPhys->add(new GeoNameTag(d.name));
        containerPhys->add(new GeoIdentifierTag(childId++));
        containerPhys->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, localZ)));
        containerPhys->add(d.volume);
    }

    return containerPhys;
}

}  // namespace SHiPGeometry
