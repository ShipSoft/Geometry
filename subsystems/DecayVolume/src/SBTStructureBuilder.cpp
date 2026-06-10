// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// SBT steel supporting structure in GeoModel, ported from the standalone
// SBTStructureBuilder. The geometry math (inclined-beam rotation frames,
// clash-avoidance segmentation and standoffs) is reproduced verbatim; only
// the wrapper is adapted: materials come from the caller, dimensions from
// SBTConfig, and the namespace/headers follow the repo conventions.
//
// H-beam cross-section decomposition
// Each H-beam is three GeoBox pieces placed as siblings in the mother volume
// (no nesting) to avoid copy-number issues in Geo2G4:
//
//       +-------------------------+  <- top flange   w  = flange width
//       |        flange           |                  tf = flange thickness
//       +----------+--------------+    web           tw = web thickness
//                  |  web              hw = h - 2*tf  (clear web height)
//       +----------+--------------+    h  = total beam height
//       |        flange           |    L  = beam length (varies per member)
//       +-------------------------+
//
// Flanges always face along X (web in the local Y-Z plane). For inclined
// Z-running beams the transform first aligns the beam axis with the required
// 3-D direction, then the cross-section orientation is preserved by the same
// rotation.

#include "DecayVolume/SBTStructureBuilder.h"

#include "DecayVolume/SBTConfig.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/Units.h>

#include <Eigen/Geometry>
#include <array>
#include <cmath>
#include <string>
#include <utility>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

namespace {

// Standoff (mm) by which a beam end stops short of the transverse row of
// beams it would otherwise cross, so the flat-placed sibling volumes do not
// overlap. Applied at both ends of the corner and longitudinal beams.
constexpr double s_beam_end_standoff_mm = 12.0;

// placeBox — add a single GeoBox as a physical-volume child of `parent`.
void placeBox(GeoVPhysVol* parent, const GeoMaterial* mat, const std::string& name, double hx,
              double hy, double hz, const GeoTrf::Transform3D& trf) {
    auto* shape = new GeoBox(hx, hy, hz);
    auto* lv = new GeoLogVol(name, shape, mat);
    auto* pv = new GeoPhysVol(lv);
    parent->add(new GeoNameTag(name));
    parent->add(new GeoTransform(trf));
    parent->add(pv);
}

// placeHBeamAlongY — H-beam whose long axis runs along global Y (vertical
// columns). Flanges face ±X (web in XZ plane); beam height runs along Z.
void placeHBeamAlongY(GeoVPhysVol* parent, const GeoMaterial* mat, const SBTConfig& cfg,
                      const std::string& name, double xc, double yc, double zc, double halfLen) {
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;
    const double hBeamHw = cfg.webHeight();

    const double hx_flange = 0.5 * hBeamW * mm;                   // flange width  -> X
    const double hz_flange = 0.5 * hBeamTf * mm;                  // flange thick  -> Z
    const double hx_web = 0.5 * cfg.hbeam_web_thickness_mm * mm;  // web thickness -> X
    const double hz_web = 0.5 * hBeamHw * mm;                     // web height    -> Z
    const double zOffset = (0.5 * hBeamH - 0.5 * hBeamTf) * mm;

    placeBox(parent, mat, name + "_FF", hx_flange, halfLen, hz_flange,
             GeoTrf::Translate3D(xc, yc, zc + zOffset));
    placeBox(parent, mat, name + "_Web", hx_web, halfLen, hz_web, GeoTrf::Translate3D(xc, yc, zc));
    placeBox(parent, mat, name + "_BkF", hx_flange, halfLen, hz_flange,
             GeoTrf::Translate3D(xc, yc, zc - zOffset));
}

// placeHBeamAlongX — H-beam whose long axis runs along global X (top/bottom
// cross-beams). Flanges face ±Y (height along Y), web stands vertically.
void placeHBeamAlongX(GeoVPhysVol* parent, const GeoMaterial* mat, const SBTConfig& cfg,
                      const std::string& name, double xc, double yc, double zc, double halfLen) {
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;
    const double hBeamHw = cfg.webHeight();

    const double hy_flange = 0.5 * hBeamTf * mm;                  // flange thickness -> Y
    const double hz_flange = 0.5 * hBeamW * mm;                   // flange width     -> Z
    const double hy_web = 0.5 * hBeamHw * mm;                     // web height       -> Y
    const double hz_web = 0.5 * cfg.hbeam_web_thickness_mm * mm;  // web thickness -> Z
    const double yOffset = (0.5 * hBeamH - 0.5 * hBeamTf) * mm;

    placeBox(parent, mat, name + "_TF", halfLen, hy_flange, hz_flange,
             GeoTrf::Translate3D(xc, yc + yOffset, zc));
    placeBox(parent, mat, name + "_Web", halfLen, hy_web, hz_web, GeoTrf::Translate3D(xc, yc, zc));
    placeBox(parent, mat, name + "_BF", halfLen, hy_flange, hz_flange,
             GeoTrf::Translate3D(xc, yc - yOffset, zc));
}

// placeHBeamInclined — H-beam between two arbitrary 3-D endpoints, flanges
// facing ±X. Builds the orthonormal frame R = [b | n | d] (columns) with
// local Z -> beam axis d, local Y -> n (height direction perpendicular to d
// and X), local X -> b, then places three boxes (flanges + web) rotated by R.
void placeHBeamInclined(GeoVPhysVol* parent, const GeoMaterial* mat, const SBTConfig& cfg,
                        const std::string& name, double x0, double y0, double z0, double x1,
                        double y1, double z1) {
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;
    const double hBeamTw = cfg.hbeam_web_thickness_mm;
    const double hBeamHw = cfg.webHeight();

    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double dz = z1 - z0;
    const double L = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double halfLen = 0.5 * L;

    if (L < 1e-6 * mm)
        return;  // degenerate — skip

    const double ux = dx / L;
    const double uy = dy / L;
    const double uz = dz / L;

    // "Height" direction of the H cross-section: flanges face ±X.
    double nx, ny, nz;
    {
        nx = 0.0;
        ny = -uz;
        nz = uy;
        double nmag = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (nmag < 1e-9) {
            // d is parallel to X -> use Y x d instead
            nx = uz;
            ny = 0.0;
            nz = -ux;
            nmag = std::sqrt(nx * nx + ny * ny + nz * nz);
        }
        nx /= nmag;
        ny /= nmag;
        nz /= nmag;
    }

    // Third axis: b = d x n  (completes right-handed frame)
    const double bx = uy * nz - uz * ny;
    const double by = uz * nx - ux * nz;
    const double bz = ux * ny - uy * nx;

    const double xcm = 0.5 * (x0 + x1);
    const double ycm = 0.5 * (y0 + y1);
    const double zcm = 0.5 * (z0 + z1);

    const double hx_flange = 0.5 * hBeamW * mm;
    const double hy_flange = 0.5 * hBeamTf * mm;
    const double hx_web = 0.5 * hBeamTw * mm;
    const double hy_web = 0.5 * hBeamHw * mm;
    const double yOffset = (0.5 * hBeamH - 0.5 * hBeamTf) * mm;

    GeoTrf::RotationMatrix3D rotMat;
    rotMat.col(0) = Eigen::Vector3d(bx, by, bz);  // local X -> b
    rotMat.col(1) = Eigen::Vector3d(nx, ny, nz);  // local Y -> n
    rotMat.col(2) = Eigen::Vector3d(ux, uy, uz);  // local Z -> d (beam axis)

    auto placePiece = [&](const std::string& pname, double hxp, double hyp, double localDY) {
        const double px = xcm + nx * localDY;
        const double py = ycm + ny * localDY;
        const double pz = zcm + nz * localDY;
        const GeoTrf::Transform3D trf =
            GeoTrf::Translate3D(px, py, pz) * GeoTrf::Transform3D(rotMat);
        auto* shape = new GeoBox(hxp, hyp, halfLen);
        auto* lv = new GeoLogVol(pname, shape, mat);
        auto* pv = new GeoPhysVol(lv);
        parent->add(new GeoNameTag(pname));
        parent->add(new GeoTransform(trf));
        parent->add(pv);
    };

    placePiece(name + "_TF", hx_flange, hy_flange, +yOffset);
    placePiece(name + "_Web", hx_web, hy_web, 0.0);
    placePiece(name + "_BF", hx_flange, hy_flange, -yOffset);
}

}  // namespace

void SBTStructureBuilder::build(GeoVPhysVol* mother, const GeoMaterial* steel, const SBTConfig& cfg,
                                const std::string& tag) {
    // Bind config values to the names the ported body uses (magnitudes in mm).
    const double xHalf_entrance = cfg.x_half_entrance_mm;
    const double yHalf_entrance = cfg.y_half_entrance_mm;
    const double xHalf_exit = cfg.x_half_exit_mm;
    const double yHalf_exit = cfg.y_half_exit_mm;
    const double totalLength = cfg.total_length_mm;
    const int nSubFrustrum = cfg.n_sub_frustum;
    const double subLength = cfg.subLength();
    const double yFloor = cfg.y_floor_mm;
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;
    const double hBeamHw = cfg.webHeight();
    const double zEntrance_mm = cfg.z_entrance_mm;

    // Linear interpolation of the X/Y half-extents at a given Z.
    auto xHalfAtZ = [&](double z_mm, double zEnt_mm) {
        const double frac = (z_mm - zEnt_mm) / totalLength;
        return xHalf_entrance + frac * (xHalf_exit - xHalf_entrance);
    };
    auto yHalfAtZ = [&](double z_mm, double zEnt_mm) {
        const double frac = (z_mm - zEnt_mm) / totalLength;
        return yHalf_entrance + frac * (yHalf_exit - yHalf_entrance);
    };

    //  (A)  VERTICAL COLUMNS — 11 rows x 2 sides, frustum top -> floor.
    for (int row = 0; row <= nSubFrustrum; ++row) {
        const double z_mm = zEntrance_mm + row * subLength;
        const double z_G = z_mm * mm;

        const double xEdge_mm = xHalfAtZ(z_mm, zEntrance_mm);
        const double yTop_mm = yHalfAtZ(z_mm, zEntrance_mm);

        const double yCol_ctr_mm = 0.5 * (yTop_mm + yFloor);
        const double yCol_half_mm = 0.5 * (yTop_mm - yFloor);

        const double yCol_ctr = yCol_ctr_mm * mm;
        const double yCol_half = yCol_half_mm * mm;

        const std::string rowTag = tag + "_Col_R" + std::to_string(row);

        placeHBeamAlongY(mother, steel, cfg, rowTag + "_PX", +xEdge_mm * mm, yCol_ctr, z_G,
                         yCol_half);
        placeHBeamAlongY(mother, steel, cfg, rowTag + "_MX", -xEdge_mm * mm, yCol_ctr, z_G,
                         yCol_half);
    }

    //  (B)  CORNER BEAMS — 4 corners, each split into nSubFrustrum segments
    //  that terminate a clear standoff short of the column they would cross.
    const std::array<std::pair<double, double>, 4> corners = {{
        {+1.0, +1.0},
        {-1.0, +1.0},
        {+1.0, -1.0},
        {-1.0, -1.0},
    }};

    for (int ci = 0; ci < 4; ++ci) {
        const double sx = corners[ci].first;
        const double sy = corners[ci].second;

        const double cbShift = sy * 0.5 * hBeamTf;  // outward Tf/2 shift
        const double cbEndGap = 0.5 * hBeamH + s_beam_end_standoff_mm;

        for (int s = 0; s < nSubFrustrum; ++s) {
            const double zA_mm = zEntrance_mm + s * subLength + cbEndGap;
            const double zB_mm = zEntrance_mm + (s + 1) * subLength - cbEndGap;

            const double xA = xHalfAtZ(zA_mm, zEntrance_mm);
            const double xB = xHalfAtZ(zB_mm, zEntrance_mm);
            const double yA = yHalfAtZ(zA_mm, zEntrance_mm);
            const double yB = yHalfAtZ(zB_mm, zEntrance_mm);

            const std::string bname =
                tag + "_CornerBeam_" + std::to_string(ci) + "_S" + std::to_string(s);

            placeHBeamInclined(mother, steel, cfg, bname, sx * xA * mm, (sy * yA + cbShift) * mm,
                               zA_mm * mm, sx * xB * mm, (sy * yB + cbShift) * mm, zB_mm * mm);
        }
    }

    //  (C)  TOP/BOTTOM LONGITUDINAL BEAMS — 1 beam/face for the narrow first
    //  half of the frustum, 2 beams/face for the wider second half. Web
    //  omitted (negligible, avoids sibling overlap). The split point tracks
    //  the configured sub-frustum count instead of assuming 10.
    const int mid = nSubFrustrum / 2;
    for (int s = 0; s < nSubFrustrum; ++s) {
        const double zLo_mm = zEntrance_mm + s * subLength;
        const double zHi_mm = zEntrance_mm + (s + 1) * subLength;

        const double xLo = xHalfAtZ(zLo_mm, zEntrance_mm);
        const double xHi = xHalfAtZ(zHi_mm, zEntrance_mm);

        const double yBeamOffset = 0.5 * hBeamHw;  // centred C-channel

        const double yTop_Lo = +yHalfAtZ(zLo_mm, zEntrance_mm) - yBeamOffset;
        const double yTop_Hi = +yHalfAtZ(zHi_mm, zEntrance_mm) - yBeamOffset;
        const double yBot_Lo = -yHalfAtZ(zLo_mm, zEntrance_mm) + yBeamOffset;
        const double yBot_Hi = -yHalfAtZ(zHi_mm, zEntrance_mm) + yBeamOffset;

        const std::string sTag = tag + "_SF" + std::to_string(s);

        for (int face = 0; face < 2; ++face) {
            const double yLo_mm = (face == 0) ? yTop_Lo : yBot_Lo;
            const double yHi_mm = (face == 0) ? yTop_Hi : yBot_Hi;
            const std::string fTag = sTag + (face == 0 ? "_Top" : "_Bot");

            auto placeLongBeam = [&](const std::string& bname, double x0, double y0, double z0,
                                     double x1, double y1, double z1) {
                const double dx = x1 - x0, dy = y1 - y0, dz_b = z1 - z0;
                const double L = std::sqrt(dx * dx + dy * dy + dz_b * dz_b);
                if (L < 1e-6 * mm)
                    return;
                const double halfLen = 0.5 * L;
                const double ux = dx / L, uy = dy / L, uz = dz_b / L;

                double nx = 0., ny = -uz, nz = uy;
                double nmag = std::sqrt(nx * nx + ny * ny + nz * nz);
                if (nmag < 1e-9) {
                    nx = uz;
                    ny = 0.;
                    nz = -ux;
                    nmag = std::sqrt(nx * nx + nz * nz);
                }
                nx /= nmag;
                ny /= nmag;
                nz /= nmag;

                GeoTrf::RotationMatrix3D rotMat;
                rotMat.col(0) =
                    Eigen::Vector3d(uy * nz - uz * ny, uz * nx - ux * nz, ux * ny - uy * nx);
                rotMat.col(1) = Eigen::Vector3d(nx, ny, nz);
                rotMat.col(2) = Eigen::Vector3d(ux, uy, uz);

                const double xcm = 0.5 * (x0 + x1), ycm = 0.5 * (y0 + y1), zcm = 0.5 * (z0 + z1);

                const double hxF = 0.5 * hBeamW * mm, hyF = 0.5 * hBeamTf * mm;
                const double yOff = (0.5 * hBeamH - 0.5 * hBeamTf) * mm;

                auto makeTrf = [&](double localDY) {
                    const double px = xcm + nx * localDY, py = ycm + ny * localDY,
                                 pz = zcm + nz * localDY;
                    return GeoTrf::Translate3D(px, py, pz) * GeoTrf::Transform3D(rotMat);
                };

                // TF (outside sensor Y range, no clash)
                {
                    auto* sh = new GeoBox(hxF, hyF, halfLen);
                    auto* lv = new GeoLogVol(bname + "_TF", sh, steel);
                    auto* pv = new GeoPhysVol(lv);
                    mother->add(new GeoNameTag(bname + "_TF"));
                    mother->add(new GeoTransform(makeTrf(+yOff)));
                    mother->add(pv);
                }
                // BF (outside sensor after centroid shift, no clash)
                {
                    auto* sh = new GeoBox(hxF, hyF, halfLen);
                    auto* lv = new GeoLogVol(bname + "_BF", sh, steel);
                    auto* pv = new GeoPhysVol(lv);
                    mother->add(new GeoNameTag(bname + "_BF"));
                    mother->add(new GeoTransform(makeTrf(-yOff)));
                    mother->add(pv);
                }
            };

            const double lbEndGap = 0.5 * hBeamW + s_beam_end_standoff_mm;
            const double zb0_mm = zLo_mm + lbEndGap;
            const double zb1_mm = zHi_mm - lbEndGap;
            const double fr0 = (zb0_mm - zLo_mm) / (zHi_mm - zLo_mm);
            const double fr1 = (zb1_mm - zLo_mm) / (zHi_mm - zLo_mm);
            const double yb0 = yLo_mm + (yHi_mm - yLo_mm) * fr0;
            const double yb1 = yLo_mm + (yHi_mm - yLo_mm) * fr1;
            const double xL0 = xLo + (xHi - xLo) * fr0;
            const double xL1 = xLo + (xHi - xLo) * fr1;

            if (s < mid) {
                placeLongBeam(fTag + "_C0", 0.0, yb0 * mm, zb0_mm * mm, 0.0, yb1 * mm, zb1_mm * mm);
            } else {
                placeLongBeam(fTag + "_C0", +(1.0 / 3.0) * xL0 * mm, yb0 * mm, zb0_mm * mm,
                              +(1.0 / 3.0) * xL1 * mm, yb1 * mm, zb1_mm * mm);
                placeLongBeam(fTag + "_C1", -(1.0 / 3.0) * xL0 * mm, yb0 * mm, zb0_mm * mm,
                              -(1.0 / 3.0) * xL1 * mm, yb1 * mm, zb1_mm * mm);
            }
        }
    }

    //  (D)  TOP/BOTTOM CROSS-BEAMS — 11 rows x 2 faces, along X.
    for (int row = 0; row <= nSubFrustrum; ++row) {
        const double z_mm = zEntrance_mm + row * subLength;
        const double z_G = z_mm * mm;

        const double xEdge_mm = xHalfAtZ(z_mm, zEntrance_mm);
        const double yTop_mm = +yHalfAtZ(z_mm, zEntrance_mm);
        const double yBot_mm = -yHalfAtZ(z_mm, zEntrance_mm);

        const std::string rowTag = tag + "_XBeam_R" + std::to_string(row);

        const double yGrowthPerZ = (yHalf_exit - yHalf_entrance) / totalLength;
        const double xbShift = (0.5 * hBeamH + 0.5 * hBeamW * yGrowthPerZ + 5.0) * mm;
        const double xbHalfLen = (xEdge_mm - 0.5 * hBeamW) * mm;

        placeHBeamAlongX(mother, steel, cfg, rowTag + "_Top", 0.0, yTop_mm * mm + xbShift, z_G,
                         xbHalfLen);
        placeHBeamAlongX(mother, steel, cfg, rowTag + "_Bot", 0.0, yBot_mm * mm - xbShift, z_G,
                         xbHalfLen);
    }
}

}  // namespace SHiPGeometry
