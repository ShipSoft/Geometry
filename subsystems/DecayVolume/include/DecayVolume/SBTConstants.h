// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>

/**
 * @brief Compile-time parameters of the Surround Background Tagger (SBT).
 *
 * Drives the steel H-beam supporting structure, the LAB scintillator sensor
 * containers, and the central helium volume of the DecayVolume subsystem.
 * All lengths are raw doubles in mm (GeoModel's native unit). Invariants
 * that used to be runtime checks are static_asserts at the bottom of this
 * header.
 */
namespace SHiPGeometry::SBT {

/**
 * @brief Every dimension the SBT geometry is parameterised by (mm).
 *
 * A literal aggregate, so a perturbed copy is itself a constant expression:
 *
 *     constexpr SBTParams taller = [] { SBTParams p = kSBT;
 *                                       p.hbeamHeight = 400.0; return p; }();
 *
 * That is what lets test_decayvolume exercise configurations other than the
 * shipped one — which is the only way to test that the helium *derivation* is
 * right in general, rather than right at today's numbers — without any of the
 * derivation becoming runtime state.
 */
struct SBTParams {
    // ── Frustum envelope ────────────────────────────────────────────────
    double xHalfEntrance = 1000.0;
    double yHalfEntrance = 1500.0;
    double xHalfExit = 2000.0;
    double yHalfExit = 3000.0;
    double totalLength = 50000.0;
    int nSubFrustum = 10;
    double yFloor = -3000.0;
    double zEntrance = -25000.0;

    // ── H-beam cross-section (HEA 260 approximation) ────────────────────
    double hbeamHeight = 250.0;
    double hbeamFlangeWidth = 260.0;
    double hbeamFlangeThickness = 12.5;
    double hbeamWebThickness = 7.5;

    // ── Sensor containers / cells ───────────────────────────────────────
    double containerThickness = 225.0;
    double cellWallThickness = 5.0;
    int nCells = 6;
    double sensorClearance = 1.0;

    // ── Helium decay region ─────────────────────────────────────────────
    // Gap left between the helium and the innermost SBT material, measured
    // along the coordinate axis. Must be >= 0; SBTEnvelope enforces that and
    // is the only consumer. It is not a safety margin — the default of 1 um is
    // simply enough to stop the helium and the SBT sharing a surface, which
    // Geant4's navigator handles badly. 0 is legal and gives exact
    // face-to-face contact.
    double heliumClearance = 0.001;
};

/// The shipped SBT. Every primitive below defaults to it, so a call site that
/// does not care about parameterisation reads exactly as it did when these
/// were loose constants.
///
/// `inline` is load-bearing, not decoration: at namespace scope `constexpr`
/// implies `const` implies internal linkage, and without `inline` every
/// translation unit would get its own kSBT — which the inline functions
/// naming it in their default arguments would then quietly violate the ODR
/// over ([basic.def.odr]/14, no diagnostic required).
inline constexpr SBTParams kSBT{};

// ── Air container enclosing the SBT structure + sensors and helium ──────
// The experiment's fixed envelope allocation for the decay region. Not an SBT
// dimension and deliberately not part of SBTParams: nothing is derived from
// it, and the three static_asserts at the bottom of this header are bounds the
// SBT must fit *inside*, not parameters it is built *from*.
inline constexpr double kEnvelopeHalfX = 2200.0;
inline constexpr double kEnvelopeHalfY = 3300.0;
inline constexpr double kEnvelopeHalfZ = 25200.0;

// ── Derived quantities ──────────────────────────────────────────────────
//
//  Convention for everything below: the parameters come last and default to
//  kSBT. Last rather than first because most of these already have a
//  meaningful leading argument (a Z, a half-width) and a defaulted parameter
//  cannot precede a non-defaulted one. The parameter is named `params`, not
//  `p`, because `p` is the conventional loop variable for a HeliumPiece in
//  SBTEnvelope and in the tests.

/// Length of one sub-frustum along Z (mm).
constexpr double subLength(const SBTParams& params = kSBT) {
    return params.totalLength / params.nSubFrustum;
}
/// Clear web height = height - 2*flange thickness (mm).
constexpr double webHeight(const SBTParams& params = kSBT) {
    return params.hbeamHeight - 2.0 * params.hbeamFlangeThickness;
}
/// Number of aluminium walls per container (nCells + 1).
constexpr int nWalls(const SBTParams& params = kSBT) {
    return params.nCells + 1;
}

// ── Frustum profile ─────────────────────────────────────────────────────
/// Exit-face Z in the DecayVolume local frame (mm).
constexpr double zExit(const SBTParams& params = kSBT) {
    return params.zEntrance + params.totalLength;
}
/// dx_half/dz of the frustum (dimensionless).
constexpr double xGrowth(const SBTParams& params = kSBT) {
    return (params.xHalfExit - params.xHalfEntrance) / params.totalLength;
}
/// dy_half/dz of the frustum (dimensionless).
constexpr double yGrowth(const SBTParams& params = kSBT) {
    return (params.yHalfExit - params.yHalfEntrance) / params.totalLength;
}
/// Half-extent of the frustum envelope in X at a given Z (mm).
constexpr double xHalfAt(double z_mm, const SBTParams& params = kSBT) {
    return params.xHalfEntrance + (z_mm - params.zEntrance) * xGrowth(params);
}
/// Half-extent of the frustum envelope in Y at a given Z (mm).
constexpr double yHalfAt(double z_mm, const SBTParams& params = kSBT) {
    return params.yHalfEntrance + (z_mm - params.zEntrance) * yGrowth(params);
}
/// Z of the start of sub-frustum @p s (mm).
constexpr double zSubLo(int s, const SBTParams& params = kSBT) {
    return params.zEntrance + s * subLength(params);
}

// ── H-beam cross-section primitives ─────────────────────────────────────
/// Offset of a flange's mid-plane from the beam axis (mm).
constexpr double hbeamFlangeOffset(const SBTParams& params = kSBT) {
    return 0.5 * params.hbeamHeight - 0.5 * params.hbeamFlangeThickness;
}
/// Reach of a flange's *outer* surface from the beam axis (mm) = h/2.
constexpr double hbeamHalfHeight(const SBTParams& params = kSBT) {
    return 0.5 * params.hbeamHeight;
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
constexpr double zSplitOffset(const SBTParams& params = kSBT) {
    return 0.5 * params.hbeamHeight + 0.5 * params.hbeamFlangeThickness;
}

// --- Side (±X) scintillator containers -------------------------------
/// Half-thickness of a side container in X (mm).
constexpr double sideContainerHalfThickness(const SBTParams& params = kSBT) {
    return 0.5 * params.containerThickness;
}
/// |X| of a side container's centroid, given the local frustum half-width.
constexpr double sideContainerCentreX(double x_half_mm, const SBTParams& params = kSBT) {
    return x_half_mm - 0.5 * params.hbeamFlangeWidth - sideContainerHalfThickness(params);
}
/// |X| of a side container's innermost face (mm).
constexpr double sideSensorInnerX(double x_half_mm, const SBTParams& params = kSBT) {
    return sideContainerCentreX(x_half_mm, params) - sideContainerHalfThickness(params);
}

// --- Top/bottom (±Y) scintillator containers -------------------------
/// Half-thickness of a top/bottom container in Y (mm).
constexpr double topBottomContainerHalfThickness(const SBTParams& params = kSBT) {
    return 0.5 * params.containerThickness - params.sensorClearance;
}
/// |Y| of a top/bottom container's centroid (mm).
constexpr double topBottomContainerCentreY(double y_half_mm, const SBTParams& params = kSBT) {
    return y_half_mm - 0.5 * params.containerThickness;
}
/// |Y| of a top/bottom container's innermost face (mm).
constexpr double topBottomSensorInnerY(double y_half_mm, const SBTParams& params = kSBT) {
    return topBottomContainerCentreY(y_half_mm, params) - topBottomContainerHalfThickness(params);
}
/// Half-extent in X available to top/bottom containers (mm).
///
/// The container stops half a flange width short of the frustum wall (that
/// is where the columns' inner face is), less a 1 mm gap so it does not
/// land flush against them. The 1 mm is inherited from the original
/// SBTSensorBuilder and is a literal, not sensorClearance — the two
/// happen to be equal at the current settings, which is worth being aware
/// of when changing sensorClearance, but they are not the same quantity
/// and this one is deliberately left as-is.
constexpr double topBottomAvailX(double x_half_mm, const SBTParams& params = kSBT) {
    return x_half_mm - 0.5 * params.hbeamFlangeWidth - 1.0;
}

// --- Top/bottom longitudinal beams -----------------------------------
//  NOTE: these beams *straddle* the scintillator plane — the outer flange
//  sits above it, the web is omitted (it would pass through the cells),
//  and the INNER FLANGE HANGS BELOW IT, INSIDE THE DECAY REGION. It, not
//  the scintillator, is the innermost material in ±Y.
/// |Y| of a top/bottom longitudinal beam's axis (mm).
constexpr double longBeamCentreY(double y_half_mm, const SBTParams& params = kSBT) {
    return y_half_mm - 0.5 * webHeight(params);
}

namespace detail {

/// Square root of @p v, usable in a constant expression.
///
/// std::sqrt is not: folding it at compile time is a GCC extension that Clang
/// does not implement, and C++26's P1383R2 has not reached libstdc++ yet. A
/// std::sqrt here therefore kept this whole header from compiling under Clang,
/// and so from being analysed by clang-tidy. Newton-Raphson to a fixpoint
/// sidesteps the question.
///
/// This is not a correctly-rounded general-purpose sqrt: the initial guess
/// suits arguments near 1, and the iteration is capped in case it settles into
/// a two-value cycle instead of a fixpoint. Every argument the SBT evaluates
/// is a taper factor 1 + g^2 with |g| well under 1, including the steeper
/// tapers test_decayvolume sweeps; the shipped one is pinned below. Do not
/// reach for it elsewhere.
constexpr double sqrtConst(double v) {
    if (v <= 0.0) {
        return 0.0;
    }
    double x = v;
    double prev = 0.0;
    for (int i = 0; i < 200 && x != prev; ++i) {
        prev = x;
        x = 0.5 * (x + v / x);
    }
    return x;
}

/// Taper factor of a top/bottom longitudinal beam, 1/cos(atan(yGrowth)).
constexpr double longBeamTaper(const SBTParams& params = kSBT) {
    const double g = yGrowth(params);
    return sqrtConst(1.0 + g * g);
}

// Bit for bit what GCC folds std::sqrt to for the shipped argument, so moving
// the computation here leaves the geometry untouched. Changing the frustum
// taper or sqrtConst means updating this pin deliberately. It pins kSBT only —
// the sweep evaluates other tapers, which no fixed bit pattern can cover.
static_assert(std::bit_cast<std::uint64_t>(longBeamTaper(kSBT)) == 0x3ff001d7c0c9d03eULL,
              "the longitudinal-beam taper factor no longer matches std::sqrt");

}  // namespace detail

/// |Y| reached by a longitudinal beam's inner flange surface (mm).
///
/// The beam is inclined by the frustum taper, so its cross-section is
/// rotated: the flange surface lies hbeamHalfHeight()/cos(atan(yGrowth))
/// from the axis measured in world Y, not hbeamHalfHeight().
constexpr double longBeamInnerY(double y_half_mm, const SBTParams& params = kSBT) {
    return longBeamCentreY(y_half_mm, params) -
           hbeamHalfHeight(params) * detail::longBeamTaper(params);
}

// ── Compile-time validation (formerly runtime throws) ──────────────────
//
//  Whether an arbitrary SBTParams is buildable at all is SBTEnvelope's
//  leavesDecayRegion(), which the tests exercise on configurations that must
//  not build. What follows constrains the *shipped* parameters only.

/// Are @p params self-consistent, before asking whether they leave a decay
/// region? Positivity and the two thickness relations, i.e. everything the
/// TOML parser used to throw on.
constexpr bool isWellFormed(const SBTParams& params = kSBT) {
    return params.totalLength > 0.0 && params.xHalfEntrance > 0.0 && params.yHalfEntrance > 0.0 &&
           params.xHalfExit > 0.0 && params.yHalfExit > 0.0 && params.hbeamFlangeWidth > 0.0 &&
           params.hbeamFlangeThickness > 0.0 && params.hbeamWebThickness > 0.0 &&
           params.containerThickness > 0.0 && params.cellWallThickness > 0.0 &&
           params.nSubFrustum > 0 && params.nCells > 0 && params.sensorClearance > 0.0 &&
           params.sensorClearance < 0.5 * params.containerThickness &&
           params.hbeamHeight > 2.0 * params.hbeamFlangeThickness && params.heliumClearance >= 0.0;
}

static_assert(isWellFormed(kSBT),
              "the shipped SBT parameters are not self-consistent: check positivity, the "
              "sensor clearance against the container thickness, and the flange thickness "
              "against the beam height");
static_assert(zSplitOffset(kSBT) < subLength(kSBT),
              "the sensor containers' flat piece no longer fits inside a sub-frustum; "
              "reduce nSubFrustum or hbeamHeight");

// The SBT structure must fit the fixed decay-volume envelope. Bounds on the
// outermost structure reach; with the current values the reaches are
// ~2130 / ~3259 / ~25130 mm, inside the 2200 / 3300 / 25200 envelope.
// X: vertical columns sit at the wider of the two frustum faces, with a
//    half-flange overhang.
// Y: top/bottom cross-beams are shifted a full beam-height above yHalfExit
//    (plus a small frustum-growth term); +10 pads the assembly standoff.
// Z: the structure spans [zEntrance, zExit()]; cross-beam flanges extend
//    half a flange-width beyond the end rows.
//
// These bound kSBT against the envelope allocation and are deliberately not
// part of isWellFormed(): the parameter sweep in test_decayvolume varies the
// frustum well past this allocation on purpose, and builds into a generous
// throwaway container precisely because of it.
static_assert(std::max(kSBT.xHalfEntrance, kSBT.xHalfExit) + 0.5 * kSBT.hbeamFlangeWidth <=
                  kEnvelopeHalfX,
              "SBT structure pierces the decay-volume envelope in X");
static_assert(kSBT.yHalfExit + kSBT.hbeamHeight + 0.5 * kSBT.hbeamFlangeWidth * yGrowth(kSBT) +
                      10.0 <=
                  kEnvelopeHalfY,
              "SBT structure pierces the decay-volume envelope in Y");
static_assert(std::max(kSBT.zEntrance < 0.0 ? -kSBT.zEntrance : kSBT.zEntrance,
                       zExit(kSBT) < 0.0 ? -zExit(kSBT) : zExit(kSBT)) +
                      0.5 * kSBT.hbeamFlangeWidth <=
                  kEnvelopeHalfZ,
              "SBT structure pierces the decay-volume envelope in Z");

}  // namespace SHiPGeometry::SBT
