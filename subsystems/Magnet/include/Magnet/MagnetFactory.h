// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

#include <string>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

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
    explicit MagnetFactory(SHiPMaterials& materials);
    ~MagnetFactory() = default;

    /**
     * @brief Build the Magnet geometry
     * @return Pointer to the physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* createYoke();
    GeoPhysVol* createCoil(const std::string& name);

    // Unit shorthand (GeoModel's native length unit is mm)
    static constexpr double mm = GeoModelKernelUnits::mm;

    // Yoke dimensions (half-sizes): outer envelope from the CAD model, the
    // inner cutout is 10 mm longer than the yoke so the subtraction leaves
    // no coincident faces.
    static constexpr double s_yokeOuterHalfX = 4000.0 * mm;
    static constexpr double s_yokeOuterHalfY = 4250.0 * mm;
    static constexpr double s_yokeOuterHalfZ = 1755.5 * mm;
    static constexpr double s_yokeInnerHalfX = 2300.0 * mm;
    static constexpr double s_yokeInnerHalfY = 3500.0 * mm;
    static constexpr double s_yokeInnerHalfZ = 1765.5 * mm;

    // Coil packs (simplified as boxes): CAD reference coil of field map V21
    static constexpr double s_coilHalfX = 65.0 * mm;
    static constexpr double s_coilHalfY = 3375.0 * mm;
    static constexpr double s_coilHalfZ = 2343.5 * mm;
    static constexpr double s_coilXOffset = 2221.0 * mm;

    // Container: the 5 m slot of the integration layout, as wide and high
    // as the yoke.
    static constexpr double s_containerHalfX = 4000.0 * mm;
    static constexpr double s_containerHalfY = 4250.0 * mm;
    static constexpr double s_containerHalfZ = 2500.0 * mm;
};

}  // namespace SHiPGeometry
