// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// SBT scintillator sensor containers in GeoModel, ported from the standalone
// SBTSensorBuilder. The GeoTrap shear math, the placement-rotation-aware
// shear-angle mapping and the Z-split clash-avoidance are reproduced
// verbatim; only the wrapper is adapted (materials from the caller,
// dimensions from SBTConfig, repo namespace/headers).
//
// Every container is a GeoTrap. The two faces (at ±dz) are rectangles; theta
// and phi describe the shear of the centroid between them. Each container
// holds 7 aluminium walls and 6 LAB cells stacked along local Z. All logical
// volumes get unique names to avoid Geo2G4 copy-number issues.

#include "DecayVolume/SBTSensorBuilder.h"

#include "DecayVolume/SBTConfig.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/Units.h>

#include <Eigen/Geometry>
#include <cmath>
#include <numbers>
#include <string>

namespace SHiPGeometry {

using namespace GeoModelKernelUnits;

namespace {

// shearAngles — returns the GeoTrap (theta, phi) shear that makes a
// container's transverse centroid drift by (worldXdrift, worldYdrift) — in
// the WORLD transverse plane — over a half-length dz, GIVEN a placement
// rotation R. Mapping the world drift into the trap local frame through
// R^-1 (= R.transpose()) keeps the stored (theta, phi) consistent with the
// placement for any rotation.
void shearAngles(const GeoTrf::RotationMatrix3D& R, double worldXdrift, double worldYdrift,
                 double dz, double& theta, double& phi) {
    const GeoTrf::Vector3D loc = R.transpose() * GeoTrf::Vector3D(worldXdrift, worldYdrift, 0.0);
    const double mag = std::hypot(loc.x(), loc.y());
    theta = std::atan2(mag, 2.0 * dz);
    phi = std::atan2(loc.y(), loc.x());
}

// Forward declaration — defined below placeSideContainer.
void placeContainerAndCells(GeoVPhysVol*, const GeoMaterial*, const GeoMaterial*, const SBTConfig&,
                            const std::string&, const GeoTrf::Transform3D&, double, double, double,
                            double, double, double);

// placeSideContainer — walls and cells of one ±X face container, Z-split at
// the column front-flange outer edge so the sensor outer face stays flush
// with the column inner flange face at all z (near piece uses a flat outer
// face; far piece resumes xHalf tracking).
void placeSideContainer(GeoVPhysVol* mother, const GeoMaterial* alMat, const GeoMaterial* labMat,
                        const SBTConfig& cfg, const std::string& name, double sgnX, double xHalf_lo,
                        double xHalf_hi, double zLo_mm, double zHi_mm, double dy1, double dy2,
                        double yCtr1, double yCtr2) {
    const double contThick = cfg.container_thickness_mm;
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;

    const double dx = 0.5 * contThick * mm;

    const double z_split_rel = 0.5 * hBeamH + 0.5 * hBeamTf;  // column front-flange outer edge
    const double z_split_mm = zLo_mm + z_split_rel;

    // ---- Piece 1: z in [zLo, z_split] — flat outer face, Y shear only ----
    {
        const double dz1 = 0.5 * (z_split_mm - zLo_mm) * mm;
        const double zMid1 = 0.5 * (zLo_mm + z_split_mm) * mm;
        const double frac1 = (z_split_mm - zLo_mm) / (zHi_mm - zLo_mm);

        const double dy_split = dy1 + (dy2 - dy1) * frac1;

        const double yC1 = yCtr1;
        const double yC2 = yCtr1 + (yCtr2 - yCtr1) * frac1;

        const double xCtr_flat = sgnX * (xHalf_lo - 0.5 * hBeamW - 0.5 * contThick) * mm;

        const double dY1 = yC2 - yC1;
        const double shear1 = std::abs(dY1);
        const double theta1 = std::atan2(shear1, 2.0 * dz1);
        const double phi1 = (dY1 >= 0.0) ? std::numbers::pi / 2.0 : -std::numbers::pi / 2.0;

        const GeoTrf::Transform3D trf1 = GeoTrf::Translate3D(xCtr_flat, 0.5 * (yC1 + yC2), zMid1);

        placeContainerAndCells(mother, alMat, labMat, cfg, name + "_P1", trf1, dz1, dy1, dy_split,
                               dx, theta1, phi1);
    }

    // ---- Piece 2: z in [z_split, zHi] — normal tracking ------------------
    {
        const double dz2 = 0.5 * (zHi_mm - z_split_mm) * mm;
        const double zMid2 = 0.5 * (z_split_mm + zHi_mm) * mm;
        const double frac1 = (z_split_mm - zLo_mm) / (zHi_mm - zLo_mm);

        const double dy_split = dy1 + (dy2 - dy1) * frac1;
        const double yC1 = yCtr1 + (yCtr2 - yCtr1) * frac1;
        const double yC2 = yCtr2;

        const double xHalf_split = xHalf_lo + (xHalf_hi - xHalf_lo) * frac1;
        const double xCtr1_p2 = sgnX * (xHalf_split - 0.5 * hBeamW - 0.5 * contThick) * mm;
        const double xCtr2_p2 = sgnX * (xHalf_hi - 0.5 * hBeamW - 0.5 * contThick) * mm;

        const double dX2 = xCtr2_p2 - xCtr1_p2;
        const double dY2 = yC2 - yC1;
        const double shear2 = std::sqrt(dX2 * dX2 + dY2 * dY2);
        const double theta2 = std::atan2(shear2, 2.0 * dz2);
        const double phi2 = std::atan2(dY2, dX2);

        const GeoTrf::Transform3D trf2 =
            GeoTrf::Translate3D(0.5 * (xCtr1_p2 + xCtr2_p2), 0.5 * (yC1 + yC2), zMid2);

        placeContainerAndCells(mother, alMat, labMat, cfg, name + "_P2", trf2, dz2, dy_split, dy2,
                               dx, theta2, phi2);
    }
}

// placeContainerAndCells — one container's nWalls aluminium walls and nCells
// LAB cells as siblings in `mother`, stacked along local Z. The container's
// (theta, phi) shear is applied per piece via a transverse centroid offset.
void placeContainerAndCells(GeoVPhysVol* mother, const GeoMaterial* alMat,
                            const GeoMaterial* labMat, const SBTConfig& cfg,
                            const std::string& name, const GeoTrf::Transform3D& trf, double dz,
                            double dy1, double dy2, double dx, double theta, double phi) {
    const double wallT = cfg.wall_thickness_mm;
    const int nCells = cfg.n_cells;
    const int nWalls = cfg.nWalls();

    const double alp = 0.0;

    const double wallHalf_z = 0.5 * wallT * mm;
    const double totalZ = 2.0 * dz;
    const double cellZ = (totalZ - nWalls * wallT * mm) / nCells;
    const double cellHalf_z = 0.5 * cellZ;

    const double step = wallT * mm + cellZ;

    auto dyAt = [&](double z_local) { return dy1 + (dy2 - dy1) * (z_local + dz) / (2.0 * dz); };

    const double tanTheta = std::tan(theta);
    auto shearOffset = [&](double z_ctr) {
        return GeoTrf::Translate3D(tanTheta * std::cos(phi) * z_ctr,
                                   tanTheta * std::sin(phi) * z_ctr, z_ctr);
    };

    // ---- 7 aluminium walls -------------------------------------------------
    for (int w = 0; w < nWalls; ++w) {
        const double z_lo = -dz + w * step;
        const double z_hi = z_lo + wallT * mm;
        const double z_ctr = 0.5 * (z_lo + z_hi);

        const double dyw1 = dyAt(z_lo);
        const double dyw2 = dyAt(z_hi);

        const std::string wname = name + "_W" + std::to_string(w);

        auto* shape = new GeoTrap(wallHalf_z, theta, phi, dyw1, dx, dx, alp, dyw2, dx, dx, alp);
        auto* lv = new GeoLogVol(wname, shape, alMat);
        auto* pv = new GeoPhysVol(lv);

        const GeoTrf::Transform3D wallTrf = trf * shearOffset(z_ctr);

        mother->add(new GeoNameTag(wname));
        mother->add(new GeoTransform(wallTrf));
        mother->add(pv);
    }

    // ---- 6 LAB cells -------------------------------------------------------
    for (int c = 0; c < nCells; ++c) {
        const double z_lo = -dz + wallT * mm + c * step;
        const double z_hi = z_lo + cellZ;
        const double z_ctr = 0.5 * (z_lo + z_hi);

        const double dyc1 = dyAt(z_lo);
        const double dyc2 = dyAt(z_hi);

        const std::string cname = name + "_C" + std::to_string(c);

        auto* shape = new GeoTrap(cellHalf_z, theta, phi, dyc1, dx, dx, alp, dyc2, dx, dx, alp);
        auto* lv = new GeoLogVol(cname, shape, labMat);
        auto* pv = new GeoFullPhysVol(lv);

        const GeoTrf::Transform3D cellTrf = trf * shearOffset(z_ctr);

        mother->add(new GeoNameTag(cname));
        mother->add(new GeoTransform(cellTrf));
        mother->add(pv);
    }
}

}  // namespace

void SBTSensorBuilder::build(GeoVPhysVol* mother, const GeoMaterial* alMat,
                             const GeoMaterial* labMat, const SBTConfig& cfg,
                             const std::string& tag) {
    // Bind config to the names the ported body uses (magnitudes in mm).
    const double xHalf_ent = cfg.x_half_entrance_mm;
    const double yHalf_ent = cfg.y_half_entrance_mm;
    const double xHalf_ext = cfg.x_half_exit_mm;
    const double yHalf_ext = cfg.y_half_exit_mm;
    const double zTotal = cfg.total_length_mm;
    const int nSub = cfg.n_sub_frustum;
    const double subLen = cfg.subLength();
    const double hBeamH = cfg.hbeam_height_mm;
    const double hBeamW = cfg.hbeam_flange_width_mm;
    const double hBeamTf = cfg.hbeam_flange_thickness_mm;
    const double contThick = cfg.container_thickness_mm;
    const double sensorClear = cfg.sensor_clearance_mm;
    const double zEntrance_mm = cfg.z_entrance_mm;

    auto xHalfAtZ = [&](double z_mm, double zEnt_mm) {
        return xHalf_ent + (z_mm - zEnt_mm) / zTotal * (xHalf_ext - xHalf_ent);
    };
    auto yHalfAtZ = [&](double z_mm, double zEnt_mm) {
        return yHalf_ent + (z_mm - zEnt_mm) / zTotal * (yHalf_ext - yHalf_ent);
    };

    //  (A)  SIDE CONTAINERS  (±X faces) — 4 containers per side, split by Y=0.
    for (int s = 0; s < nSub; ++s) {
        const double zLo_mm = zEntrance_mm + s * subLen;
        const double zHi_mm = zEntrance_mm + (s + 1) * subLen;

        const double availLo = (yHalfAtZ(zLo_mm, zEntrance_mm) - hBeamH) * mm;
        const double availHi = (yHalfAtZ(zHi_mm, zEntrance_mm) - hBeamH) * mm;

        const double dy1[4] = {availLo / 4.0, availLo / 4.0, availLo / 4.0, availLo / 4.0};
        const double dy2[4] = {availHi / 4.0, availHi / 4.0, availHi / 4.0, availHi / 4.0};

        const double yCtr1[4] = {-3.0 * availLo / 4.0, -1.0 * availLo / 4.0, +1.0 * availLo / 4.0,
                                 +3.0 * availLo / 4.0};
        const double yCtr2[4] = {-3.0 * availHi / 4.0, -1.0 * availHi / 4.0, +1.0 * availHi / 4.0,
                                 +3.0 * availHi / 4.0};

        for (int side = 0; side < 2; ++side) {
            const double sgnX = (side == 0) ? +1.0 : -1.0;

            for (int ci = 0; ci < 4; ++ci) {
                const std::string cname = tag + "_Side_S" + std::to_string(s) + "_X" +
                                          (side == 0 ? "P" : "M") + "_C" + std::to_string(ci);

                placeSideContainer(mother, alMat, labMat, cfg, cname, sgnX,
                                   xHalfAtZ(zLo_mm, zEntrance_mm), xHalfAtZ(zHi_mm, zEntrance_mm),
                                   zLo_mm, zHi_mm, dy1[ci], dy2[ci], yCtr1[ci], yCtr2[ci]);
            }
        }
    }

    //  (B)  TOP / BOTTOM CONTAINERS  (±Y faces) — 2 per face in the narrow
    //  first half of the frustum, 3 in the wider second half; split point
    //  ties to n_sub_frustum/2 to match SBTStructureBuilder. Z-split at the
    //  column front-flange outer edge like the side containers.
    const int threshold = nSub / 2;
    for (int s = 0; s < nSub; ++s) {
        const double zLo_mm = zEntrance_mm + s * subLen;
        const double zHi_mm = zEntrance_mm + (s + 1) * subLen;

        const double xAvailLo = (xHalfAtZ(zLo_mm, zEntrance_mm) - 0.5 * hBeamW - 1.0) * mm;
        const double xAvailHi = (xHalfAtZ(zHi_mm, zEntrance_mm) - 0.5 * hBeamW - 1.0) * mm;

        const double yFaceTop_Lo = (yHalfAtZ(zLo_mm, zEntrance_mm) - 0.5 * contThick) * mm;
        const double yFaceTop_Hi = (yHalfAtZ(zHi_mm, zEntrance_mm) - 0.5 * contThick) * mm;
        const double yFaceBot_Lo = -yFaceTop_Lo;
        const double yFaceBot_Hi = -yFaceTop_Hi;

        const double dx = (0.5 * contThick - sensorClear) * mm;

        const int nCont = (s < threshold) ? 2 : 3;

        const double z_split_rel = 0.5 * hBeamH + 0.5 * hBeamTf;  // column front-flange outer edge
        const double z_split_mm = zLo_mm + z_split_rel;
        const double frac_split = z_split_rel / (zHi_mm - zLo_mm);

        for (int face = 0; face < 2; ++face) {
            const double yFaceCtr1 = (face == 0) ? yFaceTop_Lo : yFaceBot_Lo;
            const double yFaceCtr2 = (face == 0) ? yFaceTop_Hi : yFaceBot_Hi;

            const double rotAngle = (face == 0) ? -std::numbers::pi / 2.0 : std::numbers::pi / 2.0;
            const GeoTrf::RotationMatrix3D rotZ =
                Eigen::AngleAxisd(rotAngle, Eigen::Vector3d::UnitZ()).toRotationMatrix();

            for (int ci = 0; ci < nCont; ++ci) {
                double localY_lo1, localY_hi1, localY_lo2, localY_hi2;
                if (nCont == 2) {
                    if (ci == 0) {
                        localY_lo1 = -xAvailLo;
                        localY_hi1 = 0.0;
                        localY_lo2 = -xAvailHi;
                        localY_hi2 = 0.0;
                    } else {
                        localY_lo1 = 0.0;
                        localY_hi1 = +xAvailLo;
                        localY_lo2 = 0.0;
                        localY_hi2 = +xAvailHi;
                    }
                } else {
                    const double t1 = xAvailLo / 3.0, t2 = xAvailHi / 3.0;
                    if (ci == 0) {
                        localY_lo1 = -xAvailLo;
                        localY_hi1 = -t1;
                        localY_lo2 = -xAvailHi;
                        localY_hi2 = -t2;
                    } else if (ci == 1) {
                        localY_lo1 = -t1;
                        localY_hi1 = +t1;
                        localY_lo2 = -t2;
                        localY_hi2 = +t2;
                    } else {
                        localY_lo1 = +t1;
                        localY_hi1 = +xAvailLo;
                        localY_lo2 = +t2;
                        localY_hi2 = +xAvailHi;
                    }
                }

                {
                    const double clr = sensorClear * mm;
                    localY_lo1 += clr;
                    localY_hi1 -= clr;
                    localY_lo2 += clr;
                    localY_hi2 -= clr;
                }

                const double lyCtr1 = 0.5 * (localY_lo1 + localY_hi1);
                const double lyCtr2 = 0.5 * (localY_lo2 + localY_hi2);

                const double lyCtr_sp = lyCtr1 + (lyCtr2 - lyCtr1) * frac_split;
                const double localY_lo_sp = localY_lo1 + (localY_lo2 - localY_lo1) * frac_split;
                const double localY_hi_sp = localY_hi1 + (localY_hi2 - localY_hi1) * frac_split;
                const double yFCtr_sp = yFaceCtr1 + (yFaceCtr2 - yFaceCtr1) * frac_split;

                const std::string cname = tag + "_TB_S" + std::to_string(s) + "_" +
                                          (face == 0 ? "Top" : "Bot") + "_C" + std::to_string(ci);

                // Piece 1: z in [zLo, z_split] — flat outer X, face Y-shear only.
                {
                    const double dz1 = 0.5 * (z_split_mm - zLo_mm) * mm;
                    const double zMid1 = 0.5 * (zLo_mm + z_split_mm) * mm;

                    const double ldy1_p = 0.5 * (localY_hi1 - localY_lo1);
                    const double ldy2_p = 0.5 * (localY_hi_sp - localY_lo_sp);
                    const double lyCtrMid = 0.5 * (lyCtr1 + lyCtr_sp);
                    const double yFCtrMid = 0.5 * (yFaceCtr1 + yFCtr_sp);

                    double theta1, phi1;
                    shearAngles(rotZ, /*worldXdrift=*/0.0,
                                /*worldYdrift=*/yFCtr_sp - yFaceCtr1, dz1, theta1, phi1);

                    const GeoTrf::Transform3D trf1 =
                        GeoTrf::Translate3D(lyCtrMid, yFCtrMid, zMid1) * GeoTrf::Transform3D(rotZ);

                    placeContainerAndCells(mother, alMat, labMat, cfg, cname + "_P1", trf1, dz1,
                                           ldy1_p, ldy2_p, dx, theta1, phi1);
                }

                // Piece 2: z in [z_split, zHi] — normal X and Y tracking.
                {
                    const double dz2 = 0.5 * (zHi_mm - z_split_mm) * mm;
                    const double zMid2 = 0.5 * (z_split_mm + zHi_mm) * mm;

                    const double ldy1_p = 0.5 * (localY_hi_sp - localY_lo_sp);
                    const double ldy2_p = 0.5 * (localY_hi2 - localY_lo2);
                    const double lyCtrMid = 0.5 * (lyCtr_sp + lyCtr2);
                    const double yFCtrMid = 0.5 * (yFCtr_sp + yFaceCtr2);

                    const double dX_loc2 = yFaceCtr2 - yFCtr_sp;  // face Y-drift
                    const double dY_loc2 = lyCtr2 - lyCtr_sp;     // container X-drift

                    double theta2, phi2;
                    shearAngles(rotZ, /*worldXdrift=*/dY_loc2, /*worldYdrift=*/dX_loc2, dz2, theta2,
                                phi2);

                    const GeoTrf::Transform3D trf2 =
                        GeoTrf::Translate3D(lyCtrMid, yFCtrMid, zMid2) * GeoTrf::Transform3D(rotZ);

                    placeContainerAndCells(mother, alMat, labMat, cfg, cname + "_P2", trf2, dz2,
                                           ldy1_p, ldy2_p, dx, theta2, phi2);
                }
            }
        }
    }
}

}  // namespace SHiPGeometry
