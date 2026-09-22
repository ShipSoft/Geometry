// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

#include <algorithm>
#include <cmath>

/**
 * @brief Compile-time parameters of the Surround Background Tagger (SBT).
 *
 * Drives the steel H-beam supporting structure, the LAB scintillator sensor
 * containers, and the central helium volume of the DecayVolume subsystem.
 * All lengths are mp-units quantities in mm. Invariants that used to be
 * runtime checks are static_asserts at the bottom of this header.
 */
namespace SHiPGeometry::SBT {

using units::LengthMm;
using units::mm;

// ── Frustum envelope ────────────────────────────────────────────────────
inline constexpr auto kXHalfEntrance = 1000.0 * mm;
inline constexpr auto kYHalfEntrance = 1500.0 * mm;
inline constexpr auto kXHalfExit = 2000.0 * mm;
inline constexpr auto kYHalfExit = 3000.0 * mm;
inline constexpr auto kTotalLength = 50000.0 * mm;
inline constexpr int kNSubFrustum = 10;
inline constexpr auto kYFloor = -3000.0 * mm;
inline constexpr auto kZEntrance = -25000.0 * mm;

// ── H-beam cross-section (HEA 260 approximation) ────────────────────────
inline constexpr auto kHBeamHeight = 250.0 * mm;
inline constexpr auto kHBeamFlangeWidth = 260.0 * mm;
inline constexpr auto kHBeamFlangeThickness = 12.5 * mm;
inline constexpr auto kHBeamWebThickness = 7.5 * mm;

// ── Sensor containers / cells ───────────────────────────────────────────
inline constexpr auto kContainerThickness = 225.0 * mm;
inline constexpr auto kWallThickness = 5.0 * mm;
inline constexpr int kNCells = 6;
inline constexpr auto kSensorClearance = 1.0 * mm;

// ── Helium decay region ─────────────────────────────────────────────────
// Gap left between the helium and the innermost SBT material, measured along
// the coordinate axis. Must be >= 0; SBTEnvelope enforces that and is the
// only consumer. It is not a safety margin — the default of 1 um is simply
// enough to stop the helium and the SBT sharing a surface, which Geant4's
// navigator handles badly. 0 is legal and gives exact face-to-face contact.
inline constexpr auto kHeliumClearance = 0.001 * mm;

// ── Air container enclosing the SBT structure + sensors and helium ──────
// The experiment's fixed envelope allocation for the decay region.
inline constexpr auto kEnvelopeHalfX = 2200.0 * mm;
inline constexpr auto kEnvelopeHalfY = 3300.0 * mm;
inline constexpr auto kEnvelopeHalfZ = 25200.0 * mm;

// ── Derived quantities ──────────────────────────────────────────────────
/// Length of one sub-frustum along Z.
constexpr LengthMm subLength() {
    return kTotalLength / kNSubFrustum;
}
/// Clear web height = height - 2*flange thickness.
constexpr LengthMm webHeight() {
    return kHBeamHeight - 2.0 * kHBeamFlangeThickness;
}
/// Number of aluminium walls per container (kNCells + 1).
constexpr int nWalls() {
    return kNCells + 1;
}

// ── Frustum profile ─────────────────────────────────────────────────────
/// Exit-face Z in the DecayVolume local frame.
constexpr LengthMm zExit() {
    return kZEntrance + kTotalLength;
}
/// dx_half/dz of the frustum (dimensionless).
constexpr double xGrowth() {
    return units::ratio((kXHalfExit - kXHalfEntrance) / kTotalLength);
}
/// dy_half/dz of the frustum (dimensionless).
constexpr double yGrowth() {
    return units::ratio((kYHalfExit - kYHalfEntrance) / kTotalLength);
}
/// Half-extent of the frustum envelope in X at a given Z.
constexpr LengthMm xHalfAt(LengthMm z) {
    return kXHalfEntrance + (z - kZEntrance) * xGrowth();
}
/// Half-extent of the frustum envelope in Y at a given Z.
constexpr LengthMm yHalfAt(LengthMm z) {
    return kYHalfEntrance + (z - kZEntrance) * yGrowth();
}
/// Z of the start of sub-frustum @p s.
constexpr LengthMm zSubLo(int s) {
    return kZEntrance + s * subLength();
}

// ── H-beam cross-section primitives ─────────────────────────────────────
/// Offset of a flange's mid-plane from the beam axis.
constexpr LengthMm hbeamFlangeOffset() {
    return 0.5 * kHBeamHeight - 0.5 * kHBeamFlangeThickness;
}
/// Reach of a flange's *outer* surface from the beam axis = h/2.
constexpr LengthMm hbeamHalfHeight() {
    return 0.5 * kHBeamHeight;
}

// ══════════════════════════════════════════════════════════════════════
//  PLACEMENT PRIMITIVES — the single source of truth.
//
//  These are THE definitions of where SBT material sits. Both builders
//  place volumes with them, and SBTEnvelope derives the helium inner
//  envelope from them, so a change to any rule below propagates to the
//  helium automatically and the two can never drift apart.
//  Do not open-code these expressions anywhere else.
// ══════════════════════════════════════════════════════════════════════

/// Z offset, from the start of a sub-frustum, of the column front-flange
/// outer edge. Sensor containers are split here: the piece upstream of it
/// must present a *flat* outer face, or it would eat into the column.
constexpr LengthMm zSplitOffset() {
    return 0.5 * kHBeamHeight + 0.5 * kHBeamFlangeThickness;
}

// --- Side (±X) scintillator containers -------------------------------
/// Half-thickness of a side container in X.
constexpr LengthMm sideContainerHalfThickness() {
    return 0.5 * kContainerThickness;
}
/// |X| of a side container's centroid, given the local frustum half-width.
constexpr LengthMm sideContainerCentreX(LengthMm xHalf) {
    return xHalf - 0.5 * kHBeamFlangeWidth - sideContainerHalfThickness();
}
/// |X| of a side container's innermost face.
constexpr LengthMm sideSensorInnerX(LengthMm xHalf) {
    return sideContainerCentreX(xHalf) - sideContainerHalfThickness();
}

// --- Top/bottom (±Y) scintillator containers -------------------------
/// Half-thickness of a top/bottom container in Y.
constexpr LengthMm topBottomContainerHalfThickness() {
    return 0.5 * kContainerThickness - kSensorClearance;
}
/// |Y| of a top/bottom container's centroid.
constexpr LengthMm topBottomContainerCentreY(LengthMm yHalf) {
    return yHalf - 0.5 * kContainerThickness;
}
/// |Y| of a top/bottom container's innermost face.
constexpr LengthMm topBottomSensorInnerY(LengthMm yHalf) {
    return topBottomContainerCentreY(yHalf) - topBottomContainerHalfThickness();
}
/// Half-extent in X available to top/bottom containers.
///
/// The container stops half a flange width short of the frustum wall (that
/// is where the columns' inner face is), less a 1 mm gap so it does not
/// land flush against them. The 1 mm is inherited from the original
/// SBTSensorBuilder and is a literal, not kSensorClearance — the two
/// happen to be equal at the current settings, which is worth being aware
/// of when changing kSensorClearance, but they are not the same quantity
/// and this one is deliberately left as-is.
constexpr LengthMm topBottomAvailX(LengthMm xHalf) {
    return xHalf - 0.5 * kHBeamFlangeWidth - 1.0 * mm;
}

// --- Top/bottom longitudinal beams -----------------------------------
//  NOTE: these beams *straddle* the scintillator plane — the outer flange
//  sits above it, the web is omitted (it would pass through the cells),
//  and the INNER FLANGE HANGS BELOW IT, INSIDE THE DECAY REGION. It, not
//  the scintillator, is the innermost material in ±Y.
/// |Y| of a top/bottom longitudinal beam's axis.
constexpr LengthMm longBeamCentreY(LengthMm yHalf) {
    return yHalf - 0.5 * webHeight();
}
/// |Y| reached by a longitudinal beam's inner flange surface.
///
/// The beam is inclined by the frustum taper, so its cross-section is
/// rotated: the flange surface lies hbeamHalfHeight()/cos(atan(yGrowth))
/// from the axis measured in world Y, not hbeamHalfHeight().
///
/// The slope factor is dimensionless double math; constexpr relies on GCC
/// folding std::sqrt in constant expressions (correctly rounded, identical
/// to the runtime result; portable constexpr sqrt arrives with C++26).
constexpr LengthMm longBeamInnerY(LengthMm yHalf) {
    const double g = yGrowth();
    return longBeamCentreY(yHalf) - hbeamHalfHeight() * std::sqrt(1.0 + g * g);
}

// ── Compile-time validation (formerly runtime throws) ──────────────────
static_assert(kTotalLength > 0.0 * mm && kXHalfEntrance > 0.0 * mm && kYHalfEntrance > 0.0 * mm &&
              kXHalfExit > 0.0 * mm && kYHalfExit > 0.0 * mm);
static_assert(kHBeamFlangeWidth > 0.0 * mm && kHBeamFlangeThickness > 0.0 * mm &&
              kHBeamWebThickness > 0.0 * mm);
static_assert(kContainerThickness > 0.0 * mm && kWallThickness > 0.0 * mm);
static_assert(kNSubFrustum > 0 && kNCells > 0);
static_assert(kSensorClearance > 0.0 * mm && kSensorClearance < 0.5 * kContainerThickness,
              "sensor clearance must be positive and leave the containers a positive thickness");
static_assert(kHBeamHeight > 2.0 * kHBeamFlangeThickness, "H-beam flanges leave no web height");
static_assert(kHeliumClearance >= 0.0 * mm,
              "a negative helium clearance overlaps the SBT by construction");
static_assert(zSplitOffset() < subLength(),
              "the sensor containers' flat piece no longer fits inside a sub-frustum; "
              "reduce kNSubFrustum or kHBeamHeight");

// The SBT structure must fit the fixed decay-volume envelope. Bounds on the
// outermost structure reach; with the current values the reaches are
// ~2130 / ~3259 / ~25130 mm, inside the 2200 / 3300 / 25200 envelope.
// X: vertical columns sit at kXHalfExit with a half-flange overhang.
// Y: top/bottom cross-beams are shifted a full beam-height above kYHalfExit
//    (plus a small frustum-growth term); +10 pads the assembly standoff.
// Z: the structure spans [kZEntrance, zExit()]; cross-beam flanges extend
//    half a flange-width beyond the end rows.
static_assert(kXHalfExit + 0.5 * kHBeamFlangeWidth <= kEnvelopeHalfX,
              "SBT structure pierces the decay-volume envelope in X");
static_assert(kYHalfExit + kHBeamHeight + 0.5 * kHBeamFlangeWidth * yGrowth() + 10.0 * mm <=
                  kEnvelopeHalfY,
              "SBT structure pierces the decay-volume envelope in Y");
static_assert(std::max(kZEntrance < 0.0 * mm ? -kZEntrance : kZEntrance,
                       zExit() < 0.0 * mm ? -zExit() : zExit()) +
                      0.5 * kHBeamFlangeWidth <=
                  kEnvelopeHalfZ,
              "SBT structure pierces the decay-volume envelope in Z");

}  // namespace SHiPGeometry::SBT
