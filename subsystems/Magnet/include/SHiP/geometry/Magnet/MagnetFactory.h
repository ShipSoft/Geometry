// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/StaticChecks.h"
#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

#include <string>

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the Magnet (spectrometer magnet) geometry
 *
 * Envelopes from the integration CAD model ST1967028_01 (layout 2026-0.1,
 * EDMS 3287817 v1.1):
 * - yoke: 8.0 m wide, 8.5 m high, 3.51 m long, centred on the 5 m slot
 * - coils: two vertical packs (reference coil of field map V21), 130 mm
 *   thick, 6.75 m high, 4.69 m long, at x = ±2221 mm
 * The yoke aperture is not resolved in the CAD model; it is kept at 7.0 m
 * high and widened to 4.6 m so that the coil packs sit inside it.
 */
class MagnetFactory {
   public:
    explicit MagnetFactory(Materials& materials);
    ~MagnetFactory() = default;

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * Z: 87.07 to 92.07 m, so the container is centred at z = 89.57 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        using units::m;
        return {.name = "Magnet",
                .node = "/SHiP/magnet",
                .id = 6,
                .x_mm = 0.0,
                .y_mm = 0.0,
                .z_mm = gm(89.57 * m),
                .isWorld = false};
    }
    /**
     * @brief Build the Magnet geometry
     * @return Pointer to the physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

    // Container: the 5 m slot of the integration layout, as wide and high
    // as the yoke.
    static constexpr auto s_containerHalfX = 4000.0 * units::mm;
    static constexpr auto s_containerHalfY = 4250.0 * units::mm;
    static constexpr auto s_containerHalfZ = 2500.0 * units::mm;

   private:
    Materials& m_materials;

    GeoPhysVol* createYoke();
    GeoPhysVol* createCoil(const std::string& name);

    // Yoke dimensions (half-sizes): outer envelope from the CAD model, the
    // inner cutout is 10 mm longer than the yoke so the subtraction leaves
    // no coincident faces.
    static constexpr auto s_yokeOuterHalfX = 4000.0 * units::mm;
    static constexpr auto s_yokeOuterHalfY = 4250.0 * units::mm;
    static constexpr auto s_yokeOuterHalfZ = 1755.5 * units::mm;
    static constexpr auto s_yokeInnerHalfX = 2300.0 * units::mm;
    static constexpr auto s_yokeInnerHalfY = 3500.0 * units::mm;
    static constexpr auto s_yokeInnerHalfZ = 1765.5 * units::mm;

    // Coil packs (simplified as boxes): CAD reference coil of field map V21
    static constexpr auto s_coilHalfX = 65.0 * units::mm;
    static constexpr auto s_coilHalfY = 3375.0 * units::mm;
    static constexpr auto s_coilHalfZ = 2343.5 * units::mm;
    static constexpr auto s_coilXOffset = 2221.0 * units::mm;

    // ── Compile-time validation ─────────────────────────────────────────
    // The coil packs sit at (±s_coilXOffset, 0, 0); checking the +x copy
    // covers both by symmetry.
    static constexpr auto s_zero = 0.0 * units::mm;
    static constexpr checks::Box s_containerBox =
        checks::box(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    static constexpr checks::Box s_coilBox =
        checks::box(s_coilXOffset, s_zero, s_zero, s_coilHalfX, s_coilHalfY, s_coilHalfZ);
    /// The iron of one yoke side: outside the inner window in x, full height.
    static constexpr checks::Box s_yokeSideBox = checks::box(
        0.5 * (s_yokeInnerHalfX + s_yokeOuterHalfX), s_zero, s_zero,
        0.5 * (s_yokeOuterHalfX - s_yokeInnerHalfX), s_yokeOuterHalfY, s_yokeOuterHalfZ);
    /// The iron above the inner window, full width.
    static constexpr checks::Box s_yokeTopBox =
        checks::box(s_zero, 0.5 * (s_yokeInnerHalfY + s_yokeOuterHalfY), s_zero, s_yokeOuterHalfX,
                    0.5 * (s_yokeOuterHalfY - s_yokeInnerHalfY), s_yokeOuterHalfZ);

    static_assert(s_yokeInnerHalfX < s_yokeOuterHalfX && s_yokeInnerHalfY < s_yokeOuterHalfY &&
                      s_yokeInnerHalfZ >= s_yokeOuterHalfZ,
                  "the yoke inner cut-out must leave a frame open along the beam");
    static_assert(checks::contains(s_containerBox, checks::box(s_yokeOuterHalfX, s_yokeOuterHalfY,
                                                               s_yokeOuterHalfZ)) &&
                      checks::contains(s_containerBox, s_coilBox),
                  "a magnet part sticks out of the container");
    static_assert(s_coilXOffset > s_coilHalfX, "the coil packs must not overlap each other");
    static_assert(!checks::overlaps(s_coilBox, s_yokeSideBox) &&
                      !checks::overlaps(s_coilBox, s_yokeTopBox),
                  "the coil packs must sit inside the yoke aperture");
};

}  // namespace SHiP::geometry
