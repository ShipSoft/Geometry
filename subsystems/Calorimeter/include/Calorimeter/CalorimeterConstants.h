// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/Units.h"

#include <array>
#include <span>

/**
 * @brief Compile-time parameters of the SHiP calorimeter geometry.
 *
 * All lengths are mp-units quantities in mm. The invariants of these
 * constants are static_asserts at the bottom of this header.
 */
namespace SHiPGeometry::Calo {

/// Layer types used in the ECAL/HCAL layer sequences.
enum class LayerCode {
    WidePVT_H,   ///< Wide PVT bar layer, bars along X (H orientation)
    WidePVT_V,   ///< Wide PVT bar layer, bars along Y (V orientation)
    ThinPS_H,    ///< Thin PS bar layer, bars along X (H orientation)
    ThinPS_V,    ///< Thin PS bar layer, bars along Y (V orientation)
    FibreHPL_Y,  ///< HPL fibre layer, fibres along Y
    FibreHPL_X,  ///< HPL fibre layer, fibres along X
    Absorber,    ///< Absorber plate (Lead in ECAL, Iron in HCAL)
    AirGap,      ///< Air gap (no volume, just advances z cursor)
};

// ── Module geometry (mm) ────────────────────────────────────────────────
inline constexpr auto kPlateXY = 2160.0 * units::mm;
inline constexpr auto kLeadThickness = 3.0 * units::mm;
inline constexpr auto kScintThickness = 10.0 * units::mm;
inline constexpr auto kHplThickness = 10.0 * units::mm;
inline constexpr auto kFiberDiameter = 1.2 * units::mm;
inline constexpr auto kFiberCoreDiameter = 1.0 * units::mm;
inline constexpr auto kIronThickness = 170.0 * units::mm;

/// Air gap within the ECAL layer sequence (mm).
inline constexpr auto kAirGap = 1000.0 * units::mm;
/// Gap between the ECAL and HCAL stacks (mm).
inline constexpr auto kGapEcalHcal = 100.0 * units::mm;

// Bar pitch (physical constant, independent of plate size)
inline constexpr auto kWidePVTBarPitch = 60.0 * units::mm;
inline constexpr auto kThinPSBarPitch = 10.0 * units::mm;

// Module tiling: modules touch (pitch = plate size)
inline constexpr int kModuleNX = 2;
inline constexpr int kModuleNY = 3;
inline constexpr auto kModulePitchX = kPlateXY;
inline constexpr auto kModulePitchY = kPlateXY;

// ── Layer sequences ─────────────────────────────────────────────────────
//
//  The `using enum` is confined to each initialiser: at namespace scope in a
//  public header it would make Absorber, AirGap, WidePVT_H ... visible
//  unqualified throughout SHiPGeometry::Calo for every includer, which is most
//  of what enum class is for.

/// ECAL layer sequence.
inline constexpr auto kEcalLayers = [] {
    using enum LayerCode;
    return std::array{
        // clang-format off
    Absorber, WidePVT_H, Absorber, WidePVT_V, Absorber, ThinPS_H, Absorber, ThinPS_V,
    Absorber, WidePVT_H, Absorber, WidePVT_V, Absorber, ThinPS_H, Absorber, ThinPS_V,
    Absorber, WidePVT_H, Absorber, WidePVT_V,
    FibreHPL_Y, FibreHPL_X, AirGap,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    FibreHPL_Y, FibreHPL_X,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    FibreHPL_Y, FibreHPL_X,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    FibreHPL_Y, FibreHPL_X,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    Absorber, ThinPS_H, Absorber, ThinPS_V, Absorber, WidePVT_H, Absorber, WidePVT_V,
    Absorber, ThinPS_H, Absorber, ThinPS_V,
    Absorber, WidePVT_H, Absorber, WidePVT_V, Absorber, ThinPS_H, Absorber, ThinPS_V,
        // clang-format on
    };
}();

/// HCAL layer sequence (Absorber means iron here rather than lead).
inline constexpr auto kHcalLayers = [] {
    using enum LayerCode;
    return std::array{
        Absorber,  WidePVT_H, Absorber,  WidePVT_V, Absorber,
        WidePVT_H, Absorber,  WidePVT_V, Absorber,  WidePVT_H,
    };
}();

// ── Fixed container envelope ────────────────────────────────────────────
// These match the SHiP subsystem envelope from subsystem_envelopes.csv
// and must not change — tests and the consistency check depend on them.
inline constexpr auto kContainerHalfX = 3000.0 * units::mm;  // 3.00 m
inline constexpr auto kContainerHalfY = 3500.0 * units::mm;  // 3.50 m
inline constexpr auto kContainerHalfZ = 1450.0 * units::mm;  // 1.45 m

// ── Derived quantities ──────────────────────────────────────────────────

/// Z advance of one layer of the given type. The absorber thickness
/// differs between the ECAL (lead) and HCAL (iron) sections.
constexpr units::LengthMm layerThickness(LayerCode code, units::LengthMm absorberThickness) {
    using enum LayerCode;
    switch (code) {
        case Absorber:
            return absorberThickness;
        case WidePVT_H:
        case WidePVT_V:
        case ThinPS_H:
        case ThinPS_V:
            return kScintThickness;
        case FibreHPL_Y:
        case FibreHPL_X:
            return kHplThickness;
        case AirGap:
            return kAirGap;
    }
    return 0.0 * units::mm;  // unreachable: all enumerators covered (-Wswitch)
}

/// Z extent of one calorimeter section.
constexpr units::LengthMm sectionZ(std::span<const LayerCode> codes,
                                   units::LengthMm absorberThickness) {
    auto z = 0.0 * units::mm;
    for (LayerCode code : codes)
        z += layerThickness(code, absorberThickness);
    return z;
}

/// Total Z extent of one ECAL+gap+HCAL stack.
inline constexpr auto kTotalStackZ =
    sectionZ(kEcalLayers, kLeadThickness) + kGapEcalHcal + sectionZ(kHcalLayers, kIronThickness);

/// Child volumes one section contributes to the container, per module. Every
/// layer places exactly one volume directly under the container (a layer
/// envelope, or the bare iron plate in the HCAL); AirGap places none and only
/// advances the z cursor.
constexpr int sectionVolumeCount(std::span<const LayerCode> codes) {
    using enum LayerCode;
    int n = 0;
    for (LayerCode code : codes)
        n += code == AirGap ? 0 : 1;
    return n;
}

/// Child volumes one module contributes to the container.
inline constexpr int kVolsPerModule =
    sectionVolumeCount(kEcalLayers) + sectionVolumeCount(kHcalLayers);

/// Bars per layer, from the plate size and the bar pitch.
inline constexpr int kWidePVTBarCount = static_cast<int>(units::ratio(kPlateXY / kWidePVTBarPitch));
inline constexpr int kThinPSBarCount = static_cast<int>(units::ratio(kPlateXY / kThinPSBarPitch));

// ── Compile-time validation ──────────────────────────────────────────────
static_assert(!kEcalLayers.empty() && !kHcalLayers.empty());
static_assert(kEcalLayers.size() == 89, "transcription guard: 89 entries in the ECAL sequence");
static_assert(kModuleNX > 0 && kModuleNY > 0);
static_assert(kFiberCoreDiameter > 0.0 * units::mm && kFiberCoreDiameter <= kFiberDiameter);
static_assert(kWidePVTBarCount * kWidePVTBarPitch == kPlateXY,
              "plate size must be a whole number of wide PVT bars");
static_assert(kThinPSBarCount * kThinPSBarPitch == kPlateXY,
              "plate size must be a whole number of thin PS bars");
static_assert(kTotalStackZ == 2600.0 * units::mm,
              "transcription guard: 40 lead + 40 scint + 8 HPL + 1 air gap in the ECAL "
              "(1600 mm), 100 mm gap, 5 iron + 5 scint in the HCAL (900 mm)");
static_assert(kTotalStackZ <= 2.0 * kContainerHalfZ, "layer stack exceeds the container in Z");
static_assert(0.5 * kPlateXY + 0.5 * (kModuleNX - 1) * kModulePitchX <= kContainerHalfX,
              "module array exceeds the container in X");
static_assert(0.5 * kPlateXY + 0.5 * (kModuleNY - 1) * kModulePitchY <= kContainerHalfY,
              "module array exceeds the container in Y");

}  // namespace SHiPGeometry::Calo
