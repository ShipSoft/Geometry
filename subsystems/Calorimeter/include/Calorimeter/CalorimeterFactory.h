// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

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

    // Component dimensions (convert m to mm)
    // Note: GeoModel uses mm internally, so 1m = 1000mm

    // ECAL front
    static constexpr double s_ecalFrontHalfX = 2250.0;  // 2.25 m
    static constexpr double s_ecalFrontHalfY = 3500.0;  // 3.50 m
    static constexpr double s_ecalFrontHalfZ = 100.0;   // 0.10 m
    static constexpr double s_ecalFrontZ = 96970.0;     // 96.97 m

    // ECAL back
    static constexpr double s_ecalBackHalfX = 2750.0;  // 2.75 m
    static constexpr double s_ecalBackHalfY = 3500.0;  // 3.50 m
    static constexpr double s_ecalBackHalfZ = 300.0;   // 0.30 m
    static constexpr double s_ecalBackZ = 98370.0;     // 98.37 m

    // HCAL
    static constexpr double s_hcalHalfX = 3000.0;  // 3.00 m
    static constexpr double s_hcalHalfY = 3500.0;  // 3.50 m
    static constexpr double s_hcalHalfZ = 500.0;   // 0.50 m
    static constexpr double s_hcalZ = 99270.0;     // 99.27 m

    // Container dimensions (spans all components)
    // From CSV: ECAL front 96.87-97.07, ECAL back 98.07-98.67, HCAL 98.77-99.77
    // Full span: 96.87 to 99.77 m → centre: 98.32 m, half-length: 1.45 m
    static constexpr double s_containerHalfX = s_hcalHalfX;  // Use largest X
    static constexpr double s_containerHalfY = s_hcalHalfY;  // Use largest Y
    static constexpr double s_containerHalfZ = 1450.0;       // 1.45 m
    static constexpr double s_containerCentreZ = 98320.0;    // 98.32 m
};

}  // namespace SHiPGeometry
