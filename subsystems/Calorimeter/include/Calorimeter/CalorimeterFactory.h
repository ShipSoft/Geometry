// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <algorithm>
#include <array>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Calorimeter (electromagnetic + hadronic calorimeter) geometry
 *
 * Creates a container with three sections:
 * - ECAL front: Z 96.87-97.07 m → centre 96.97 m, half-length 0.10 m, 2.25×3.50 m
 * - ECAL back: Z 98.07-98.67 m → centre 98.37 m, half-length 0.30 m, 2.75×3.50 m
 * - HCAL: Z 98.77-99.77 m → centre 99.27 m, half-length 0.50 m, 3.00×3.50 m
 */
class CalorimeterFactory {
   public:
    explicit CalorimeterFactory(SHiPMaterials& materials);
    ~CalorimeterFactory() = default;

    /**
     * @brief Build the Calorimeter geometry
     * @return Pointer to container volume with ECAL front, ECAL back, and HCAL
     */
    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    struct ComponentParams {
        const char* name;
        int id;
        double halfX;
        double halfY;
        double halfZ;
        double centreZ;  // absolute position in mm
    };

    // Note: GeoModel uses mm internally, so 1 m = 1000 mm
    static constexpr std::array kComponents{
        ComponentParams{"/SHiP/calorimeter/ecal_front", 0, 2250.0, 3500.0, 100.0, 96970.0},
        ComponentParams{"/SHiP/calorimeter/ecal_back", 1, 2750.0, 3500.0, 300.0, 98370.0},
        ComponentParams{"/SHiP/calorimeter/hcal", 2, 3000.0, 3500.0, 500.0, 99270.0},
    };

    // Container dimensions computed from component extents
    static constexpr double kContainerHalfX =
        std::ranges::max(kComponents, {}, &ComponentParams::halfX).halfX;
    static constexpr double kContainerHalfY =
        std::ranges::max(kComponents, {}, &ComponentParams::halfY).halfY;

    static constexpr double kZMin = [] {
        double minimum = kComponents[0].centreZ - kComponents[0].halfZ;
        for (const auto& c : kComponents)
            minimum = std::min(minimum, c.centreZ - c.halfZ);
        return minimum;
    }();
    static constexpr double kZMax = [] {
        double maximum = kComponents[0].centreZ + kComponents[0].halfZ;
        for (const auto& c : kComponents)
            maximum = std::max(maximum, c.centreZ + c.halfZ);
        return maximum;
    }();
    static constexpr double kContainerCentreZ = (kZMin + kZMax) / 2.0;
    static constexpr double kContainerHalfZ = (kZMax - kZMin) / 2.0;
};

}  // namespace SHiPGeometry
