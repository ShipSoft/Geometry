// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "DecayVolume/SBTEnvelope.h"

#include "DecayVolume/SBTConfig.h"

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoTrap.h>
#include <GeoModelKernel/Units.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace SHiPGeometry {

namespace {

// Index of the sub-frustum containing z (clamped at the exit face).
int subFrustumAt(const SBTConfig& cfg, double z_mm) {
    const double f = (z_mm - cfg.z_entrance_mm) / cfg.subLength();
    return std::clamp(static_cast<int>(std::floor(f)), 0, cfg.n_sub_frustum - 1);
}

// The half-width the SIDE containers track at z. Inside the flat piece of a
// sub-frustum they are frozen at that sub-frustum's entrance half-width (see
// SBTSensorBuilder::placeSideContainer), which is what makes the X envelope a
// sawtooth rather than a straight line.
double sideTrackedXHalf(const SBTConfig& cfg, double z_mm) {
    const int s = subFrustumAt(cfg, z_mm);
    const double zLo = cfg.zSubLo(s);
    if (z_mm <= zLo + cfg.zSplitOffset())
        return cfg.xHalfAt(zLo);
    return cfg.xHalfAt(z_mm);
}

}  // namespace

double innerFreeHalfX(const SBTConfig& cfg, double z_mm) {
    // Side scintillator containers. (Columns and corner beams are outboard of
    // these by construction: their inner face is at x_half - flange_width/2,
    // a full container_thickness further out.)
    return cfg.sideSensorInnerX(sideTrackedXHalf(cfg, z_mm));
}

double innerFreeHalfY(const SBTConfig& cfg, double z_mm) {
    const double yHalf = cfg.yHalfAt(z_mm);
    // Top/bottom scintillator containers ...
    const double sensor = cfg.topBottomSensorInnerY(yHalf);
    // ... and the longitudinal beams' inner flange, which hangs below them.
    const double beam = cfg.longBeamInnerY(yHalf);
    // (Cross-beams sit a full beam-height above y_half and never reach in.)
    //
    // KNOWN LIMITATION (conservative): the longitudinal beams do not run the
    // full sub-frustum length — they stand off by ~0.5*flange_width from each
    // boundary (SBTStructureBuilder). In those ~142 mm end bands the true Y
    // bound is the shallower scintillator, but we cap at the deeper beam line
    // regardless, so the helium is up to ~13.5 mm short of the material there.
    // This never overlaps (it only makes the helium smaller), but it does leave
    // the helium slightly inside the "no unphysical margin" ideal over ~2.8 m of
    // length — worth ~0.18 m^3, i.e. 0.04% of the fiducial volume. Reclaiming it
    // means subdividing the Y envelope at the beam ends, which has to account
    // for the flange box's z-projection overshooting its nominal end; deferred
    // to a dedicated change rather than risking that interaction here.
    return std::min(sensor, beam);
}

std::vector<double> envelopeKnots(const SBTConfig& cfg) {
    // The knot layout below assumes each sub-frustum is long enough to contain
    // its flat piece. If the beams grow (or n_sub_frustum does) until that
    // stops being true, the sensor builder itself already produces an inverted
    // second piece, so fail here rather than emit unordered knots and build a
    // silently wrong helium.
    if (cfg.zSplitOffset() >= cfg.subLength()) {
        throw std::runtime_error(
            "SBTEnvelope: zSplitOffset (" + std::to_string(cfg.zSplitOffset()) +
            " mm) >= subLength (" + std::to_string(cfg.subLength()) +
            " mm): the sensor containers' flat piece no longer fits inside a sub-frustum. "
            "Reduce n_sub_frustum or hbeam_height.");
    }

    std::vector<double> knots;
    knots.reserve(2 * static_cast<std::size_t>(cfg.n_sub_frustum) + 1);
    for (int s = 0; s < cfg.n_sub_frustum; ++s) {
        knots.push_back(cfg.zSubLo(s));                       // sub-frustum boundary
        knots.push_back(cfg.zSubLo(s) + cfg.zSplitOffset());  // end of the flat piece
    }
    knots.push_back(cfg.zExit());
    return knots;
}

namespace {

// The X envelope STEPS at each zSplitOffset knot: the flat piece's inner face
// sits at x_half(zLo)-..., and the tracking piece that follows begins at
// x_half(zSplit)-..., xGrowth*zSplitOffset further out. Sampling the outboard
// branch at the knot would put the helium's leading edge in the same plane as
// the flat piece's inner face — no overlap in the strict sense, but a
// coincident surface, which Geant4's navigator will not thank us for, and
// which silently eats the clearance besides.
//
// So evaluate the envelope as a *closed* set: at a knot, take the smaller of
// the two one-sided limits. The helium is then continuous, strictly inscribed,
// and keeps its full clearance everywhere.
double envelopeAtKnot(const SBTConfig& cfg, double z_mm, bool isX) {
    constexpr double kEps = 1e-6;
    const double lo = std::max(z_mm - kEps, cfg.z_entrance_mm);
    const double hi = std::min(z_mm + kEps, cfg.zExit());
    return isX ? std::min(innerFreeHalfX(cfg, lo), innerFreeHalfX(cfg, hi))
               : std::min(innerFreeHalfY(cfg, lo), innerFreeHalfY(cfg, hi));
}

}  // namespace

std::vector<HeliumPiece> heliumPieces(const SBTConfig& cfg) {
    if (cfg.helium_clearance_mm < 0.0) {
        throw std::runtime_error("SBTEnvelope: helium_clearance_mm is negative (" +
                                 std::to_string(cfg.helium_clearance_mm) +
                                 "), which would build a helium that overlaps the SBT by "
                                 "construction.");
    }

    const std::vector<double> knots = envelopeKnots(cfg);
    const double clr = cfg.helium_clearance_mm;

    std::vector<HeliumPiece> pieces;
    pieces.reserve(knots.size() - 1);

    for (std::size_t i = 0; i + 1 < knots.size(); ++i) {
        const double zLo = knots[i];
        const double zHi = knots[i + 1];
        if (zHi - zLo < 1e-9)
            continue;

        HeliumPiece p;
        p.z_lo_mm = zLo;
        p.z_hi_mm = zHi;
        p.dx_lo_mm = envelopeAtKnot(cfg, zLo, /*isX=*/true) - clr;
        p.dx_hi_mm = envelopeAtKnot(cfg, zHi, /*isX=*/true) - clr;
        p.dy_lo_mm = envelopeAtKnot(cfg, zLo, /*isX=*/false) - clr;
        p.dy_hi_mm = envelopeAtKnot(cfg, zHi, /*isX=*/false) - clr;

        if (p.dx_lo_mm <= 0.0 || p.dx_hi_mm <= 0.0 || p.dy_lo_mm <= 0.0 || p.dy_hi_mm <= 0.0) {
            throw std::runtime_error(
                "SBTEnvelope: the configured SBT leaves no decay region between z=" +
                std::to_string(zLo) + " and z=" + std::to_string(zHi) +
                " mm (helium half-extents " + std::to_string(p.dx_lo_mm) + "/" +
                std::to_string(p.dy_lo_mm) +
                " mm). The containers, beams or clearances in sbt.toml are too large for the "
                "frustum.");
        }
        pieces.push_back(p);
    }
    return pieces;
}

void buildHelium(GeoPhysVol* container, const GeoMaterial* helium, const SBTConfig& cfg) {
    using namespace GeoModelKernelUnits;

    const std::vector<HeliumPiece> pieces = heliumPieces(cfg);
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        const HeliumPiece& p = pieces[i];
        const double dz = 0.5 * (p.z_hi_mm - p.z_lo_mm) * mm;
        const double zMid = 0.5 * (p.z_lo_mm + p.z_hi_mm) * mm;
        const double dx1 = p.dx_lo_mm * mm;
        const double dy1 = p.dy_lo_mm * mm;
        const double dx2 = p.dx_hi_mm * mm;
        const double dy2 = p.dy_hi_mm * mm;

        const std::string name = "/SHiP/decay_volume/helium_" + std::to_string(i);
        auto* shape = new GeoTrap(dz, 0.0, 0.0, dy1, dx1, dx1, 0.0, dy2, dx2, dx2, 0.0);
        auto* log = new GeoLogVol(name, shape, helium);
        auto* phys = new GeoPhysVol(log);
        container->add(new GeoNameTag(name));
        container->add(new GeoTransform(GeoTrf::Translate3D(0.0, 0.0, zMid)));
        container->add(phys);
    }
}

}  // namespace SHiPGeometry
