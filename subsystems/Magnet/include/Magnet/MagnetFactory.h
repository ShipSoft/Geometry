// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>
#include "SHiPGeometry/SubsystemRegistry.h"

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


    /// This subsystem's self-description (name, node, id, placement).
    static SubsystemDescriptor descriptor() {
        return {"Magnet", "/SHiP/magnet", 6, 0.0, 0.0, 89570.0, false};
    }
    /**
     * @brief Build the Magnet geometry
     * @return Pointer to the physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    GeoPhysVol* createYoke();
    GeoPhysVol* createCoil(const std::string& name);
    GeoPhysVol* createVerticalConnector(const std::string& name);

    // Unit shorthand (GeoModel's native length unit is mm)
    static constexpr double mm = GeoModelKernelUnits::mm;

    // Yoke dimensions from GDML (half-sizes)
    // GDML: outer 600×860×280 cm, inner 440×700×282 cm
    static constexpr double s_yokeOuterHalfX = 3000.0 * mm;
    static constexpr double s_yokeOuterHalfY = 4300.0 * mm;
    static constexpr double s_yokeOuterHalfZ = 1400.0 * mm;
    static constexpr double s_yokeInnerHalfX = 2200.0 * mm;
    static constexpr double s_yokeInnerHalfY = 3500.0 * mm;
    static constexpr double s_yokeInnerHalfZ = 1410.0 * mm;

    // Coil dimensions (simplified as boxes)
    static constexpr double s_coilHalfX = 800.0 * mm;
    static constexpr double s_coilHalfY = 400.0 * mm;
    static constexpr double s_coilHalfZ = 1660.0 * mm;
    static constexpr double s_coilXOffset = 2200.0 * mm;
    static constexpr double s_coilYOffset = 3250.0 * mm;

    // Vertical connector dimensions from GDML: 80×650×25 cm
    static constexpr double s_connectorHalfX = 400.0 * mm;
    static constexpr double s_connectorHalfY = 3250.0 * mm;
    static constexpr double s_connectorHalfZ = 125.0 * mm;
    static constexpr double s_connectorXOffset = 2600.0 * mm;
    static constexpr double s_connectorZOffset = 1525.0 * mm;  // From GDML positions

    // Container sized to enclose the yoke, coils, and connectors: HalfX and
    // HalfZ exceed the yoke outer dimensions, while HalfY matches it.
    static constexpr double s_containerHalfX = 3250.0 * mm;
    static constexpr double s_containerHalfY = 4300.0 * mm;
    static constexpr double s_containerHalfZ = 2500.0 * mm;
};

}  // namespace SHiPGeometry
