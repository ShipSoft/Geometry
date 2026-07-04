// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "NeutrinoDetector/NeutrinoDetectorFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoSerialIdentifier.h>
#include <GeoModelKernel/GeoSerialTransformer.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTube.h>
#include <GeoModelKernel/GeoXF.h>
#include <GeoModelKernel/Units.h>

#include <GeoGenericFunctions/Variable.h>
#include <cmath>
#include <string>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

// ── file-scope geometry parameters (mm) and helpers ──────────────────────────
namespace {

constexpr const char* kBase = "/SHiP/neutrino_detector";

// Veto: upstream PVT bar station (3 planes × 7 bars).
constexpr double s_veto_bar_length = 420.0;
constexpr double s_veto_bar_width = 60.0;
constexpr double s_veto_bar_thickness = 10.0;
constexpr int s_veto_n_bars = 7;
constexpr double s_veto_shift_y = 9.0;    // ±stagger of planes 0/1 in Y
constexpr double s_veto_plane_gap = 5.0;  // air gap between plane faces
constexpr double s_veto_gap_to_target = 20.0;
constexpr double s_veto_plane_xy = s_veto_n_bars * s_veto_bar_width;  // 420
constexpr double s_veto_total_thickness =
    3.0 * s_veto_bar_thickness + 2.0 * s_veto_plane_gap;  // 40

// NuTarget: silicon/tungsten sampling target (120 layers).
constexpr double s_tgt_plate_xy = 400.0;
constexpr double s_tgt_w = 3.5;
constexpr double s_tgt_si = 0.300;
constexpr double s_tgt_w_si_gap = 0.200;
constexpr double s_tgt_si_si_gap = 6.800;
constexpr int s_tgt_n_layers = 120;
constexpr double s_tgt_pitch =
    s_tgt_w + s_tgt_w_si_gap + s_tgt_si + s_tgt_si_si_gap + s_tgt_si + s_tgt_w_si_gap;  // 11.3

// HCAL: 3 transverse sections, 14 layers each.
constexpr double s_hcal_gap_to_target = 10.0;
constexpr double s_hcal_iron = 50.0;
constexpr double s_hcal_fibre_diameter = 0.250;  // round scintillating fibre
constexpr int s_hcal_fibre_sublayers = 2;        // staggered sub-layers per plane
constexpr double s_hcal_fibre_plane =
    s_hcal_fibre_sublayers * s_hcal_fibre_diameter;  // 0.5 mm fibre-plane thickness
constexpr double s_hcal_tile_xy = 50.0;
constexpr double s_hcal_tile_thickness = 10.0;
constexpr double s_hcal_layer =
    s_hcal_iron + 2.0 * s_hcal_fibre_plane + s_hcal_tile_thickness;  // 61
constexpr int s_hcal_n_sections = 3;
constexpr int s_hcal_n_layers = 14;
constexpr double s_hcal_section_xy[s_hcal_n_sections] = {400.0, 500.0, 600.0};

/// Place one shared log volume as a child with a per-instance name, copy id,
/// and position (coordinates in mm; converted to GeoModel units here).
void placeChild(GeoVPhysVol* mother, const GeoLogVol* log, const std::string& name, int id,
                double x_mm, double y_mm, double z_mm) {
    mother->add(new GeoNameTag(name));
    mother->add(new GeoIdentifierTag(id));
    mother->add(new GeoTransform(GeoTrf::Translate3D(x_mm * mm, y_mm * mm, z_mm * mm)));
    mother->add(new GeoPhysVol(log));
}

/// Build the upstream PVT veto: three planes of seven bars, laid out working
/// upstream from @p zDownstreamFace_mm (planes 0/1 horizontal and staggered in
/// Y, plane 2 vertical).
void buildVeto(GeoVPhysVol* mother, const GeoMaterial* pvt, double zDownstreamFace_mm) {
    const double halfL = 0.5 * s_veto_bar_length * mm;
    const double halfW = 0.5 * s_veto_bar_width * mm;
    const double halfT = 0.5 * s_veto_bar_thickness * mm;

    auto* horizLog =
        new GeoLogVol(std::string(kBase) + "/veto/bar_h", new GeoBox(halfL, halfW, halfT), pvt);
    auto* vertLog =
        new GeoLogVol(std::string(kBase) + "/veto/bar_v", new GeoBox(halfW, halfL, halfT), pvt);

    const double zc[3] = {zDownstreamFace_mm - 2.5 * s_veto_bar_thickness - 2.0 * s_veto_plane_gap,
                          zDownstreamFace_mm - 1.5 * s_veto_bar_thickness - 1.0 * s_veto_plane_gap,
                          zDownstreamFace_mm - 0.5 * s_veto_bar_thickness};
    const double yShift[3] = {+s_veto_shift_y, -s_veto_shift_y, 0.0};

    for (int p = 0; p < 3; ++p) {
        const bool vertical = (p == 2);
        for (int b = 0; b < s_veto_n_bars; ++b) {
            const double stack =
                -0.5 * s_veto_plane_xy + 0.5 * s_veto_bar_width + b * s_veto_bar_width;
            const double x = vertical ? stack : 0.0;
            const double y = vertical ? 0.0 : stack + yShift[p];
            const std::string name =
                std::string(kBase) + "/veto/P" + std::to_string(p) + "_B" + std::to_string(b);
            placeChild(mother, vertical ? vertLog : horizLog, name, p * 10 + b, x, y, zc[p]);
        }
    }
}

/// Build the silicon/tungsten target: 120 layers of a tungsten plate followed
/// by an X and a Y silicon plane, placed directly in the container starting at
/// @p zStart_mm.
void buildTarget(GeoVPhysVol* mother, const GeoMaterial* tungsten, const GeoMaterial* silicon,
                 double zStart_mm) {
    const double halfXY = 0.5 * s_tgt_plate_xy * mm;
    auto* wLog = new GeoLogVol(std::string(kBase) + "/target/W_plate",
                               new GeoBox(halfXY, halfXY, 0.5 * s_tgt_w * mm), tungsten);
    auto* siXLog = new GeoLogVol(std::string(kBase) + "/target/Si_X",
                                 new GeoBox(halfXY, halfXY, 0.5 * s_tgt_si * mm), silicon);
    auto* siYLog = new GeoLogVol(std::string(kBase) + "/target/Si_Y",
                                 new GeoBox(halfXY, halfXY, 0.5 * s_tgt_si * mm), silicon);

    double z = zStart_mm;
    for (int l = 0; l < s_tgt_n_layers; ++l) {
        const std::string tag = std::string(kBase) + "/target/L" + std::to_string(l);
        placeChild(mother, wLog, tag + "/W", l, 0.0, 0.0, z + 0.5 * s_tgt_w);
        z += s_tgt_w + s_tgt_w_si_gap;
        placeChild(mother, siXLog, tag + "/Si_X", l, 0.0, 0.0, z + 0.5 * s_tgt_si);
        z += s_tgt_si + s_tgt_si_si_gap;
        placeChild(mother, siYLog, tag + "/Si_Y", l, 0.0, 0.0, z + 0.5 * s_tgt_si);
        z += s_tgt_si + s_tgt_w_si_gap;
    }
}

/// Build one scintillating-fibre plane: an air envelope filled with individual
/// polystyrene fibres in two staggered sub-layers. Fibres run along X (X plane,
/// measuring Y) or along Y (Y plane, measuring X). @p envLog is the shared
/// per-section envelope log volume and @p fibrePhys the shared per-section fibre
/// physical volume; @p zFront_mm is the plane's upstream face.
void buildFibrePlane(GeoVPhysVol* mother, const GeoLogVol* envLog, GeoPhysVol* fibrePhys, bool isX,
                     double sectionXY_mm, double zFront_mm, const std::string& lTag) {
    const double r = 0.5 * s_hcal_fibre_diameter;
    const double d = s_hcal_fibre_diameter;
    const double halfXY = 0.5 * sectionXY_mm;
    const double envHalfZ = s_hcal_fibre_sublayers * r;  // 0.25 mm

    const std::string envName = lTag + (isX ? "/FibreX" : "/FibreY");
    auto* envPhys = new GeoPhysVol(envLog);
    mother->add(new GeoNameTag(envName));
    mother->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, (zFront_mm + envHalfZ) * mm)));
    mother->add(envPhys);

    // Tube axis is Z; rotate it to lie along X (X plane) or Y (Y plane).
    const GeoTrf::Transform3D rot =
        isX ? GeoTrf::Transform3D(GeoTrf::RotateY3D(-90.0 * deg))  // Z → X
            : GeoTrf::Transform3D(GeoTrf::RotateX3D(90.0 * deg));  // Z → Y

    const int nFibres = static_cast<int>(2.0 * halfXY / d);

    // Every fibre is geometrically identical, so a single shared fibre physical
    // volume (@p fibrePhys, shared across every plane in the section) is placed
    // nFibres times per sub-layer via a GeoSerialTransformer: O(nFibres) tree
    // nodes collapse to one. The unit step is 1 mm along the measuring axis
    // (Y for an X plane, X for a Y plane); Pow(step, i) advances by i·d.
    // GeoSerialIdentifier reproduces the original continuous channel numbering
    // (sub-layer 1 continues after sub-layer 0).
    const GeoTrf::Transform3D unitStep =
        isX ? GeoTrf::Transform3D(GeoTrf::Translate3D(0.0, mm, 0.0))
            : GeoTrf::Transform3D(GeoTrf::Translate3D(mm, 0.0, 0.0));

    for (int sub = 0; sub < s_hcal_fibre_sublayers; ++sub) {
        const double localZ = -envHalfZ + r + sub * d;
        const double stagger = (sub == 1) ? r : 0.0;
        const double start = -halfXY + r + stagger;

        GeoGenfun::Variable i;
        GeoGenfun::GENFUNCTION pos = start + d * i;  // fibre centre along measuring axis (mm)
        GeoXF::TRANSFUNCTION fibreXF =
            GeoTrf::Transform3D(GeoTrf::Translate3D(0.0, 0.0, localZ * mm)) *
            GeoXF::Pow(unitStep, pos) * rot;

        envPhys->add(new GeoNameTag(envName));
        envPhys->add(new GeoSerialIdentifier(sub * nFibres));
        envPhys->add(new GeoSerialTransformer(fibrePhys, &fibreXF, nFibres));
    }
}

/// Build the HCAL: three transverse sections of 14 layers each, every layer an
/// iron absorber, an X and a Y scintillating-fibre plane (individual fibres in
/// two staggered sub-layers), and a polystyrene tile grid, starting at
/// @p zStart_mm.
void buildHCal(GeoVPhysVol* mother, const GeoMaterial* air, const GeoMaterial* iron,
               const GeoMaterial* polystyrene, double zStart_mm) {
    // Tile shape is section-independent → one shared log volume.
    auto* tileLog = new GeoLogVol(std::string(kBase) + "/hcal/tile",
                                  new GeoBox(0.5 * s_hcal_tile_xy * mm, 0.5 * s_hcal_tile_xy * mm,
                                             0.5 * s_hcal_tile_thickness * mm),
                                  polystyrene);

    const double r = 0.5 * s_hcal_fibre_diameter;
    const double envHalfZ = s_hcal_fibre_sublayers * r;

    double z = zStart_mm + s_hcal_gap_to_target;
    int tileId = 0;

    for (int s = 0; s < s_hcal_n_sections; ++s) {
        const double sectionXY = s_hcal_section_xy[s];
        const double halfXY = 0.5 * sectionXY * mm;
        // Fibre-plane air envelope is one fibre diameter larger than the section
        // so the staggered sub-layer is fully contained.
        const double envHalfXY = (0.5 * sectionXY + s_hcal_fibre_diameter) * mm;
        const std::string sTag = std::string(kBase) + "/hcal/S" + std::to_string(s);

        // Per-section shared log volumes (reused across all 14 layers).
        auto* ironLog =
            new GeoLogVol(sTag + "/Fe", new GeoBox(halfXY, halfXY, 0.5 * s_hcal_iron * mm), iron);
        auto* fibreEnvLog = new GeoLogVol(sTag + "/Fibre_ENV",
                                          new GeoBox(envHalfXY, envHalfXY, envHalfZ * mm), air);
        auto* fibreLog =
            new GeoLogVol(sTag + "/fibre", new GeoTube(0.0, r * mm, halfXY), polystyrene);
        // One shared fibre physical volume placed by every plane in this section.
        auto* fibrePhys = new GeoPhysVol(fibreLog);

        const int nTiles = static_cast<int>(std::lround(sectionXY / s_hcal_tile_xy));
        const double tileHalf = 0.5 * s_hcal_tile_xy;

        for (int l = 0; l < s_hcal_n_layers; ++l) {
            const std::string lTag = sTag + "_L" + std::to_string(l);

            placeChild(mother, ironLog, lTag + "/Fe", l, 0.0, 0.0, z + 0.5 * s_hcal_iron);
            z += s_hcal_iron;
            buildFibrePlane(mother, fibreEnvLog, fibrePhys, /*isX=*/true, sectionXY, z, lTag);
            z += s_hcal_fibre_plane;
            buildFibrePlane(mother, fibreEnvLog, fibrePhys, /*isX=*/false, sectionXY, z, lTag);
            z += s_hcal_fibre_plane;

            const double tileZc = z + 0.5 * s_hcal_tile_thickness;
            for (int row = 0; row < nTiles; ++row) {
                const double y = -0.5 * sectionXY + tileHalf + row * s_hcal_tile_xy;
                for (int col = 0; col < nTiles; ++col) {
                    const double x = -0.5 * sectionXY + tileHalf + col * s_hcal_tile_xy;
                    const std::string name =
                        lTag + "/tile_" + std::to_string(col) + "_" + std::to_string(row);
                    placeChild(mother, tileLog, name, tileId++, x, y, tileZc);
                }
            }
            z += s_hcal_tile_thickness;
        }
    }
}

}  // namespace

/// Construct the factory against the shared materials catalogue.
NeutrinoDetectorFactory::NeutrinoDetectorFactory(SHiPMaterials& materials)
    : m_materials(materials) {}

/// Resolve materials, create the air container, lay out the veto, target and
/// HCAL in beam order (content centred in the container), and return it.
GeoPhysVol* NeutrinoDetectorFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial("Air");
    const GeoMaterial* tungsten = m_materials.requireMaterial("Tungsten");
    const GeoMaterial* silicon = m_materials.requireMaterial("Silicon");
    const GeoMaterial* iron = m_materials.requireMaterial("Iron");
    const GeoMaterial* polystyrene = m_materials.requireMaterial("Polystyrene");
    const GeoMaterial* pvt = m_materials.requireMaterial("PVT");

    auto* containerBox = new GeoBox(s_halfX * mm, s_halfY * mm, s_halfZ * mm);
    auto* containerLog = new GeoLogVol(kBase, containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    // Longitudinal layout (upstream → downstream), content centred in the container.
    const double targetDepth = s_tgt_pitch * s_tgt_n_layers;
    const double hcalDepth =
        s_hcal_gap_to_target + s_hcal_n_sections * (s_hcal_layer * s_hcal_n_layers);
    const double contentDepth =
        s_veto_total_thickness + s_veto_gap_to_target + targetDepth + hcalDepth;
    const double zStart = -0.5 * contentDepth;

    const double vetoDownstreamFace = zStart + s_veto_total_thickness;
    const double targetStart = vetoDownstreamFace + s_veto_gap_to_target;
    const double hcalStart = targetStart + targetDepth;

    buildVeto(containerPhys, pvt, vetoDownstreamFace);
    buildTarget(containerPhys, tungsten, silicon, targetStart);
    buildHCal(containerPhys, air, iron, polystyrene, hcalStart);

    return containerPhys;
}

}  // namespace SHiPGeometry
