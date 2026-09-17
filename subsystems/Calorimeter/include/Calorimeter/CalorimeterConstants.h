// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

#include <array>
#include <span>

/**
 * @brief Compile-time parameters of the SHiP calorimeter geometry.
 *
 * All lengths are raw doubles in mm (GeoModel's native unit). Invariants
 * that used to be runtime checks are static_asserts at the bottom of this
 * header.
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
inline constexpr double kPlateXY = 2160.0;
inline constexpr double kLeadThickness = 3.0;
inline constexpr double kScintThickness = 10.0;
inline constexpr double kHplThickness = 10.0;
inline constexpr double kFiberDiameter = 1.2;
inline constexpr double kFiberCoreDiameter = 1.0;
inline constexpr double kIronThickness = 170.0;

/// Air gap within the ECAL layer sequence (mm).
inline constexpr double kAirGap = 1000.0;
/// Gap between the ECAL and HCAL stacks (mm).
inline constexpr double kGapEcalHcal = 100.0;

// Bar pitch (physical constant, independent of plate size)
inline constexpr double kWidePVTBarPitch = 60.0;
inline constexpr double kThinPSBarPitch = 10.0;

// Module tiling: modules touch (pitch = plate size)
inline constexpr int kModuleNX = 2;
inline constexpr int kModuleNY = 3;
inline constexpr double kModulePitchX = kPlateXY;
inline constexpr double kModulePitchY = kPlateXY;

// ── Layer sequences ─────────────────────────────────────────────────────
using enum LayerCode;

/// ECAL layer sequence.
inline constexpr std::array kEcalLayers{
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

/// HCAL layer sequence (Absorber means iron here rather than lead).
inline constexpr std::array kHcalLayers{
    Absorber,  WidePVT_H, Absorber,  WidePVT_V, Absorber,
    WidePVT_H, Absorber,  WidePVT_V, Absorber,  WidePVT_H,
};

// ── Fixed container envelope ────────────────────────────────────────────
// These match the SHiP subsystem envelope from subsystem_envelopes.csv
// and must not change — tests and the consistency check depend on them.
inline constexpr double kContainerHalfX = 3000.0 * GeoModelKernelUnits::mm;  // 3.00 m
inline constexpr double kContainerHalfY = 3500.0 * GeoModelKernelUnits::mm;  // 3.50 m
inline constexpr double kContainerHalfZ = 1450.0 * GeoModelKernelUnits::mm;  // 1.45 m

// ── Derived quantities ──────────────────────────────────────────────────

/// Z advance of one layer of the given type (mm). The absorber thickness
/// differs between the ECAL (lead) and HCAL (iron) sections.
constexpr double layerThickness(LayerCode code, double absorberThickness) {
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
    return 0.0;  // unreachable: all enumerators covered (-Wswitch)
}

/// Z extent of one calorimeter section (mm).
constexpr double sectionZ(std::span<const LayerCode> codes, double absorberThickness) {
    double z = 0.0;
    for (LayerCode code : codes)
        z += layerThickness(code, absorberThickness);
    return z;
}

/// Total Z extent of one ECAL+gap+HCAL stack (mm).
inline constexpr double kTotalStackZ =
    sectionZ(kEcalLayers, kLeadThickness) + kGapEcalHcal + sectionZ(kHcalLayers, kIronThickness);

/// Bars per layer, from the plate size and the bar pitch.
inline constexpr int kWidePVTBarCount = static_cast<int>(kPlateXY / kWidePVTBarPitch);
inline constexpr int kThinPSBarCount = static_cast<int>(kPlateXY / kThinPSBarPitch);

// ── Compile-time validation (formerly runtime throws) ───────────────────
static_assert(!kEcalLayers.empty() && !kHcalLayers.empty());
static_assert(kEcalLayers.size() == 89, "transcription guard: 89 entries in the ECAL sequence");
static_assert(kModuleNX > 0 && kModuleNY > 0);
static_assert(kFiberCoreDiameter > 0.0 && kFiberCoreDiameter <= kFiberDiameter);
static_assert(kWidePVTBarCount * kWidePVTBarPitch == kPlateXY,
              "plate size must be a whole number of wide PVT bars");
static_assert(kThinPSBarCount * kThinPSBarPitch == kPlateXY,
              "plate size must be a whole number of thin PS bars");
static_assert(kTotalStackZ <= 2.0 * kContainerHalfZ, "layer stack exceeds the container in Z");
static_assert(0.5 * kPlateXY + 0.5 * (kModuleNX - 1) * kModulePitchX <= kContainerHalfX,
              "module array exceeds the container in X");
static_assert(0.5 * kPlateXY + 0.5 * (kModuleNY - 1) * kModulePitchY <= kContainerHalfY,
              "module array exceeds the container in Y");

}  // namespace SHiPGeometry::Calo
