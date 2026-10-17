// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

#include <mp-units/framework.h>
#include <mp-units/systems/si.h>

/**
 * @brief Typed-unit vocabulary for the SHiP geometry.
 *
 * Dimensional constants are mp-units quantities stored in the unit they are
 * declared in (an mm-declared value is a quantity of mm, an m-declared value
 * a quantity of m, ...). gm() is the single bridge into GeoModel's
 * raw-double unit system: it reads the stored number back in its own unit
 * (an identity, no conversion) and multiplies by the matching
 * GeoModelKernelUnits factor — the exact floating-point operation the
 * former `value * GeoModelKernelUnits::<unit>` annotation performed — so
 * retyping leaves the built geometry bit-for-bit unchanged.
 */
namespace SHiPGeometry::units {

// Curated symbol set. Deliberately NOT `using namespace si::unit_symbols;`,
// which would inject ~100 one/two-letter names (T, K, h, min, N, ...).
inline constexpr auto mm = mp_units::si::unit_symbols::mm;
inline constexpr auto cm = mp_units::si::unit_symbols::cm;
inline constexpr auto m = mp_units::si::unit_symbols::m;
inline constexpr auto deg = mp_units::si::unit_symbols::deg;
inline constexpr auto rad = mp_units::si::unit_symbols::rad;
inline constexpr auto g = mp_units::si::unit_symbols::g;
inline constexpr auto mol = mp_units::si::unit_symbols::mol;
inline constexpr auto cm3 = mp_units::cubic(mp_units::si::centi<mp_units::si::metre>);
using mp_units::one;

/// Spellings for struct members and function parameters. Constants instead
/// use `static constexpr auto x = 1.0 * units::mm;` and deduce their type.
using LengthMm = mp_units::quantity<mm>;
using LengthCm = mp_units::quantity<cm>;
using AngleDeg = mp_units::quantity<deg>;
using AngleRad = mp_units::quantity<rad>;

/// Length quantity -> GeoModel raw double (GeoModel native unit: mm).
template <auto U, typename Rep>
    requires(U == mm || U == cm || U == m)
[[nodiscard]] constexpr double gm(mp_units::quantity<U, Rep> q) {
    if constexpr (U == mm) {
        return q.numerical_value_ref_in(U) * GeoModelKernelUnits::mm;  // * 1.0
    } else if constexpr (U == cm) {
        return q.numerical_value_ref_in(U) * GeoModelKernelUnits::cm;  // * 10.0
    } else {
        return q.numerical_value_ref_in(U) * GeoModelKernelUnits::m;  // * 1000.0
    }
}

/// Angle quantity -> GeoModel raw double (GeoTrf / GeoShape take radians).
template <auto U, typename Rep>
    requires(U == deg || U == rad)
[[nodiscard]] constexpr double gm(mp_units::quantity<U, Rep> q) {
    if constexpr (U == deg) {
        return q.numerical_value_ref_in(U) * GeoModelKernelUnits::degree;
    } else {
        return q.numerical_value_ref_in(U) * GeoModelKernelUnits::radian;  // * 1.0
    }
}

/// Density bridge; reproduces the former `(X * g) / cm3` evaluation order.
[[nodiscard]] constexpr double gm(mp_units::quantity<g / cm3> density) {
    return density.numerical_value_ref_in(g / cm3) * GeoModelKernelUnits::g /
           GeoModelKernelUnits::cm3;
}

/// Molar-mass bridge; reproduces the former `(X * g) / mole` evaluation order.
[[nodiscard]] constexpr double gm(mp_units::quantity<g / mol> molarMass) {
    return molarMass.numerical_value_ref_in(g / mol) * GeoModelKernelUnits::g /
           GeoModelKernelUnits::mole;
}

/// Dimensionless quantity -> plain double (counts, growth slopes, and inputs
/// to GeoGenfun / Eigen, which own their own double math).
[[nodiscard]] constexpr double ratio(mp_units::quantity<one> q) {
    return q.numerical_value_in(one);
}

}  // namespace SHiPGeometry::units
