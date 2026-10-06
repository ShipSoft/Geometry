// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiPGeometry/StaticChecks.h"
#include "SHiPGeometry/Units.h"

#include <string>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Magnet (spectrometer magnet) geometry
 *
 * Based on GDML reference:
 * - magyoke (iron yoke): Outer 600×860×280 cm, inner cutout 440×700×282 cm
 * - MCoil1-4: Aluminium half-tubes, rmin=10mm, rmax=800mm, half-z=1660mm
 * - CV connectors: Aluminium boxes 80×650×25 cm at 4 positions
 */
class MagnetFactory {
   public:
    explicit MagnetFactory(SHiPMaterials& materials);
    ~MagnetFactory() = default;

    /**
     * @brief Build the Magnet geometry
     * @return Pointer to the physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

    // Container sized to enclose the yoke, coils, and connectors: HalfX and
    // HalfZ exceed the yoke outer dimensions, while HalfY matches it.
    static constexpr auto s_containerHalfX = 3250.0 * units::mm;
    static constexpr auto s_containerHalfY = 4300.0 * units::mm;
    static constexpr auto s_containerHalfZ = 2500.0 * units::mm;

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* createYoke();
    GeoPhysVol* createCoil(const std::string& name);
    GeoPhysVol* createVerticalConnector(const std::string& name);

    // Yoke dimensions from GDML (half-sizes)
    // GDML: outer 600×860×280 cm, inner 440×700×282 cm
    static constexpr auto s_yokeOuterHalfX = 3000.0 * units::mm;
    static constexpr auto s_yokeOuterHalfY = 4300.0 * units::mm;
    static constexpr auto s_yokeOuterHalfZ = 1400.0 * units::mm;
    static constexpr auto s_yokeInnerHalfX = 2200.0 * units::mm;
    static constexpr auto s_yokeInnerHalfY = 3500.0 * units::mm;
    static constexpr auto s_yokeInnerHalfZ = 1410.0 * units::mm;

    // Coil dimensions (simplified as boxes)
    static constexpr auto s_coilHalfX = 800.0 * units::mm;
    static constexpr auto s_coilHalfY = 400.0 * units::mm;
    static constexpr auto s_coilHalfZ = 1660.0 * units::mm;
    static constexpr auto s_coilXOffset = 2200.0 * units::mm;
    static constexpr auto s_coilYOffset = 3250.0 * units::mm;

    // Vertical connector dimensions from GDML: 80×650×25 cm
    static constexpr auto s_connectorHalfX = 400.0 * units::mm;
    static constexpr auto s_connectorHalfY = 3250.0 * units::mm;
    static constexpr auto s_connectorHalfZ = 125.0 * units::mm;
    static constexpr auto s_connectorXOffset = 2600.0 * units::mm;
    static constexpr auto s_connectorZOffset = 1525.0 * units::mm;  // From GDML positions

    // ── Compile-time validation ─────────────────────────────────────────
    // Coils sit at (±s_coilXOffset, ±s_coilYOffset, 0) and connectors at
    // (±s_connectorXOffset, 0, ±s_connectorZOffset); checking the +x/+y/+z
    // copy covers all four by symmetry.
    static constexpr auto s_zero = 0.0 * units::mm;
    static constexpr checks::Box s_containerBox =
        checks::box(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    static constexpr checks::Box s_coilBox =
        checks::box(s_coilXOffset, s_coilYOffset, s_zero, s_coilHalfX, s_coilHalfY, s_coilHalfZ);
    static constexpr checks::Box s_connectorBox =
        checks::box(s_connectorXOffset, s_zero, s_connectorZOffset, s_connectorHalfX,
                    s_connectorHalfY, s_connectorHalfZ);
    /// The iron of one yoke side: outside the inner window in x, full height.
    static constexpr checks::Box s_yokeSideBox = checks::box(
        0.5 * (s_yokeInnerHalfX + s_yokeOuterHalfX), s_zero, s_zero,
        0.5 * (s_yokeOuterHalfX - s_yokeInnerHalfX), s_yokeOuterHalfY, s_yokeOuterHalfZ);

    static_assert(s_yokeInnerHalfX < s_yokeOuterHalfX && s_yokeInnerHalfY < s_yokeOuterHalfY &&
                      s_yokeInnerHalfZ >= s_yokeOuterHalfZ,
                  "the yoke inner cut-out must leave a frame open along the beam");
    static_assert(checks::contains(s_containerBox, checks::box(s_yokeOuterHalfX, s_yokeOuterHalfY,
                                                               s_yokeOuterHalfZ)) &&
                      checks::contains(s_containerBox, s_coilBox) &&
                      checks::contains(s_containerBox, s_connectorBox),
                  "a magnet part sticks out of the container");
    static_assert(!checks::overlaps(s_connectorBox, s_yokeSideBox),
                  "the connectors must sit clear of the yoke");
    // Known deviations: the box coils approximate GDML half-tubes and overlap
    // both the yoke sides and the connectors. Asserted so a fix is noticed.
    static_assert(checks::overlaps(s_coilBox, s_yokeSideBox) &&
                      checks::overlaps(s_coilBox, s_connectorBox),
                  "the coils no longer overlap the yoke and connectors: assert !overlaps");
};

}  // namespace SHiPGeometry
