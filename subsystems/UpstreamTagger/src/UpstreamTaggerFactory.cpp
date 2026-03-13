// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "UpstreamTagger/SHiPUBTManager.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/Units.h>

#include <cmath>
#include <stdexcept>
#include <string>

using namespace GeoModelKernelUnits;

namespace SHiPGeometry {

namespace {

// ============================================================================
//  placeTubeSubLayer  (file-scope helper)
//
//  Wall and gas are placed as SIBLINGS in the envelope (not nested) to avoid
//  Geo2G4 copy-number corruption that occurs when a GeoPhysVol is a daughter
//  of another GeoPhysVol sharing the same GeoLogVol across many instances.
// ============================================================================
void placeTubeSubLayer(GeoPhysVol* envelope, GeoLogVol* wallLog, GeoLogVol* gasLog,
                       double halfExtentY, double zLocal, double yShift, double rOuter_mm,
                       const std::string& tag, int subIdx, SHiPUBTManager* manager) {
    const double rOuter = rOuter_mm * mm;
    const double pitch = 2.0 * rOuter;

    const double usable = 2.0 * halfExtentY - std::abs(yShift);
    const int nTubes = std::max(1, static_cast<int>(std::floor(usable / pitch)) + 1);
    const double yFirst = -0.5 * (nTubes - 1) * pitch + yShift;

    const GeoTrf::Transform3D rotToX = GeoTrf::RotateY3D(-90.0 * deg);

    for (int i = 0; i < nTubes; ++i) {
        const double yPos = yFirst + i * pitch;
        if (std::abs(yPos) > halfExtentY + 1e-6)
            continue;

        const std::string name = tag + "_S" + std::to_string(subIdx) + "_T" + std::to_string(i);
        const GeoTrf::Transform3D trf = GeoTrf::Translate3D(0.0, yPos, zLocal) * rotToX;

        // Wall (not sensitive) — placed first
        auto* wallPV = new GeoPhysVol(wallLog);
        envelope->add(new GeoNameTag((name + "_Wall").c_str()));
        envelope->add(new GeoTransform(trf));
        envelope->add(wallPV);

        // Gas (sensitive) — sibling of wall, same transform
        auto* gasFPV = new GeoFullPhysVol(gasLog);
        envelope->add(new GeoNameTag((name + "_Gas").c_str()));
        envelope->add(new GeoTransform(trf));
        envelope->add(gasFPV);

        if (manager)
            manager->addTubeGasVolume(gasFPV);
    }
}

void placeDoubleStaggeredLayer(GeoPhysVol* envelope, GeoLogVol* wallLog, GeoLogVol* gasLog,
                               double halfExtentY, double zCenter, double rOuter_mm,
                               const std::string& tag, SHiPUBTManager* manager) {
    const double rOuter = rOuter_mm * mm;
    placeTubeSubLayer(envelope, wallLog, gasLog, halfExtentY, zCenter - rOuter, 0.0, rOuter_mm, tag,
                      0, manager);
    placeTubeSubLayer(envelope, wallLog, gasLog, halfExtentY, zCenter + rOuter, rOuter, rOuter_mm,
                      tag, 1, manager);
}

GeoPhysVol* makeEnvelope(GeoPhysVol* mother, const GeoMaterial* mat, const std::string& tag,
                         double hx, double hy, double hz, double cx, double cy, double cz) {
    auto* log = new GeoLogVol((tag + "_LOG").c_str(), new GeoBox(hx, hy, hz),
                              const_cast<GeoMaterial*>(mat));
    auto* pv = new GeoPhysVol(log);
    mother->add(new GeoNameTag(tag.c_str()));
    mother->add(new GeoTransform(GeoTrf::Translate3D(cx, cy, cz)));
    mother->add(pv);
    return pv;
}

}  // anonymous namespace

// ============================================================================
//  UpstreamTaggerFactory
// ============================================================================
UpstreamTaggerFactory::UpstreamTaggerFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoVPhysVol* UpstreamTaggerFactory::build(SHiPUBTManager* manager) {
    // ---- Materials ----------------------------------------------------------
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* mylar = m_materials.requireMaterial("Mylar");
    const GeoMaterial* arco2 = m_materials.requireMaterial("ArCO2");
    const GeoMaterial* poly = m_materials.requireMaterial("Polystyrene");

    // ---- Tube cross-section -------------------------------------------------
    const double rOuter = s_tubeROuter_mm * mm;
    const double rInner = (s_tubeROuter_mm - s_tubeWall_mm) * mm;
    const double rGas = rInner - 0.001 * mm;  // 1 µm clearance
    const double halfLen = s_tubeHalfLen_mm * mm;

    if (rInner <= 0.0 || rInner >= rOuter)
        throw std::runtime_error("UBT: invalid tube wall thickness");

    // ---- Tile half-size -----------------------------------------------------
    const double tileHalf = 0.5 * s_tileSide_mm * mm;

    // ---- Shared logical volumes (one wall+gas pair per region) --------------
    auto makeTubeLVs = [&](const std::string& region) -> std::pair<GeoLogVol*, GeoLogVol*> {
        auto* wLV =
            new GeoLogVol(("UBT_" + region + "_TubeWall_LV").c_str(),
                          new GeoTube(rInner, rOuter, halfLen), const_cast<GeoMaterial*>(mylar));
        auto* gLV = new GeoLogVol(("UBT_" + region + "_TubeGas_LV").c_str(),
                                  new GeoTube(0.0, rGas, halfLen), const_cast<GeoMaterial*>(arco2));
        return {wLV, gLV};
    };

    auto* tileLV = new GeoLogVol("UBT_Tile_LV", new GeoBox(tileHalf, tileHalf, tileHalf),
                                 const_cast<GeoMaterial*>(poly));

    // ---- Layout constants scaled to CSV envelope (all mm) -------------------
    // CSV: half_width=750mm, half_height=1600mm, halfZ(length/2)=200mm
    //
    // Outer band: y=[+200,+1600] top / y=[-1600,-200] bottom
    constexpr double outerHalfY_mm = 700.0;  // (1600-200)/2
    constexpr double outerCtrY_mm = 900.0;   // (200+1600)/2
    // Left half-plane:  x=[-750,+200] → halfX=475, ctrX=-275
    constexpr double leftHalfX_mm = 475.0;
    constexpr double leftCtrX_mm = -275.0;
    // Right half-plane: x=[-200,+750] → halfX=475, ctrX=+275
    constexpr double rightHalfX_mm = 475.0;
    constexpr double rightCtrX_mm = +275.0;
    // Z offset between left/right half-planes
    constexpr double zLeft_mm = -2.0 * s_tubeROuter_mm;
    constexpr double zRight_mm = +2.0 * s_tubeROuter_mm;
    constexpr double outerEnvHalfZ_mm = 3.0 * s_tubeROuter_mm + 0.5;
    // Central strip: x=[-600,+600], y=[-200,+200]
    constexpr double ctrHalfX_mm = 600.0;
    constexpr double ctrHalfY_mm = 200.0;
    constexpr double ctrEnvHalfZ_mm = s_tubeROuter_mm + 0.5;
    // Tile blocks: x=[-750,-600] left, x=[+600,+750] right
    constexpr double tileBlkHalfX_mm = 75.0;  // (750-600)/2
    constexpr double tileBlkHalfY_mm = 200.0;
    constexpr double tileBlkCtrX_mm = 675.0;  // 600+75

    // ---- Top-level envelope -------------------------------------------------
    auto* envPV = new GeoPhysVol(new GeoLogVol("UBT_Envelope_LV",
                                               new GeoBox(s_halfX * mm, s_halfY * mm, s_halfZ * mm),
                                               const_cast<GeoMaterial*>(air)));

    // ---- Outer bands (top + bottom) -----------------------------------------
    for (int sign : {+1, -1}) {
        const std::string band = (sign > 0) ? "Top" : "Bottom";
        const double yCtr = sign * outerCtrY_mm * mm;

        // Left half-plane
        {
            auto [wLV, gLV] = makeTubeLVs(band + "_Left");
            auto* env = makeEnvelope(envPV, air, "UBT_" + band + "_Left", leftHalfX_mm * mm,
                                     outerHalfY_mm * mm, outerEnvHalfZ_mm * mm, leftCtrX_mm * mm,
                                     yCtr, zLeft_mm * mm);
            placeDoubleStaggeredLayer(env, wLV, gLV, outerHalfY_mm * mm, 0.0, s_tubeROuter_mm,
                                      "UBT_" + band + "_Left", manager);
        }
        // Right half-plane
        {
            auto [wLV, gLV] = makeTubeLVs(band + "_Right");
            auto* env = makeEnvelope(envPV, air, "UBT_" + band + "_Right", rightHalfX_mm * mm,
                                     outerHalfY_mm * mm, outerEnvHalfZ_mm * mm, rightCtrX_mm * mm,
                                     yCtr, zRight_mm * mm);
            placeDoubleStaggeredLayer(env, wLV, gLV, outerHalfY_mm * mm, 0.0, s_tubeROuter_mm,
                                      "UBT_" + band + "_Right", manager);
        }
    }

    // ---- Central tube strip -------------------------------------------------
    {
        auto [wLV, gLV] = makeTubeLVs("Central");
        auto* env = makeEnvelope(envPV, air, "UBT_Central", ctrHalfX_mm * mm, ctrHalfY_mm * mm,
                                 ctrEnvHalfZ_mm * mm, 0.0, 0.0, 0.0);
        placeDoubleStaggeredLayer(env, wLV, gLV, ctrHalfY_mm * mm, 0.0, s_tubeROuter_mm,
                                  "UBT_Central", manager);
    }

    // ---- Tile blocks --------------------------------------------------------
    auto placeTileBlock = [&](const std::string& blkTag, double ctrX) {
        auto* blkEnv = makeEnvelope(envPV, air, blkTag, tileBlkHalfX_mm * mm, tileBlkHalfY_mm * mm,
                                    tileHalf, ctrX, 0.0, 0.0);

        const int nX = static_cast<int>(std::round(2.0 * tileBlkHalfX_mm / s_tileSide_mm));
        const int nY = static_cast<int>(std::round(2.0 * tileBlkHalfY_mm / s_tileSide_mm));
        const double xFirst = -(nX - 1) * 0.5 * s_tileSide_mm * mm;
        const double yFirst = -(nY - 1) * 0.5 * s_tileSide_mm * mm;

        for (int ix = 0; ix < nX; ++ix) {
            for (int iy = 0; iy < nY; ++iy) {
                const std::string tname =
                    blkTag + "_T" + std::to_string(ix) + "_" + std::to_string(iy);
                auto* tileFPV = new GeoFullPhysVol(tileLV);
                blkEnv->add(new GeoNameTag(tname.c_str()));
                blkEnv->add(new GeoTransform(GeoTrf::Translate3D(
                    xFirst + ix * s_tileSide_mm * mm, yFirst + iy * s_tileSide_mm * mm, 0.0)));
                blkEnv->add(tileFPV);
                if (manager)
                    manager->addTileVolume(tileFPV);
            }
        }
    };

    placeTileBlock("UBT_TileLeft", -tileBlkCtrX_mm * mm);
    placeTileBlock("UBT_TileRight", +tileBlkCtrX_mm * mm);

    return envPV;
}

}  // namespace SHiPGeometry
