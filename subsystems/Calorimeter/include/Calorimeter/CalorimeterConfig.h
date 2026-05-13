// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>
#include <vector>

namespace SHiPGeometry {

/// Layer type codes used in the ECAL/HCAL layer sequences.
enum class LayerCode : int {
    WidePVT_H = 1,   ///< Wide PVT bar layer, bars along X (H orientation)
    WidePVT_V = 2,   ///< Wide PVT bar layer, bars along Y (V orientation)
    ThinPS_H = 3,    ///< Thin PS bar layer, bars along X (H orientation)
    ThinPS_V = 4,    ///< Thin PS bar layer, bars along Y (V orientation)
    FibreHPL_Y = 5,  ///< HPL fibre layer, fibres along Y
    FibreHPL_X = 6,  ///< HPL fibre layer, fibres along X
    Absorber = 7,    ///< Absorber plate (Lead in ECAL, Iron in HCAL)
    AirGap = 8,      ///< Air gap (no volume, just advances z cursor)
};

/**
 * @brief Configuration for the SHiP calorimeter geometry.
 *
 * Populated by readCaloConfig() from a calo.toml file.
 *
 * Layer codes (layers / layers2 vectors):
 *   1 = WidePVT bar layer, bars along X  (H orientation)
 *   2 = WidePVT bar layer, bars along Y  (V orientation)
 *   3 = ThinPS  bar layer, bars along X  (H orientation)
 *   4 = ThinPS  bar layer, bars along Y  (V orientation)
 *   5 = HPL fibre layer, fibres along Y
 *   6 = HPL fibre layer, fibres along X
 *   7 = absorber plate  (Lead in ECAL section, Iron in HCAL section)
 *   8 = air gap (no volume placed, just advances z cursor)
 *
 * layers  → ECAL section
 * layers2 → HCAL section (code 7 means Iron here)
 */
struct CalorimeterConfig {
    // ECAL layer sequence
    std::vector<int> layers;

    // Individual layer thicknesses (mm)
    double plate_xy_mm = 2160.0;
    double lead_thickness_mm = 3.0;
    double scint_thickness_mm = 10.0;
    double hpl_thickness_mm = 10.0;
    double fiber_diameter_mm = 1.2;
    double fiber_core_diameter_mm = -1.0;  // <0 → use fiber_diameter_mm
    double airgap_mm = 1000.0;

    // HCAL layer sequence and absorber thickness
    std::vector<int> layers2;
    double iron_thickness_mm = 170.0;

    // Gap between ECAL and HCAL stacks (mm)
    double gap_ecal_hcal_mm = 0.0;

    // Module tiling
    int module_nx = 1;
    int module_ny = 1;
    double module_pitch_x_mm = 0.0;  // 0 → use plate_xy_mm
    double module_pitch_y_mm = 0.0;  // 0 → use plate_xy_mm

    // Tolerance (envelope padding, mm)
    double tol_x_mm = 10.0;
    double tol_y_mm = 10.0;
    double tol_z_mm = 10.0;

    // Global position of the calorimeter stack centre in the world (mm)
    double detector_offset_x_mm = 0.0;
    double detector_offset_y_mm = 0.0;
    double detector_offset_z_mm = 96970.0;

    // If true, the layer stack is centred at z=0 in the container volume.
    bool center_stack = true;
};

/**
 * @brief Parse a calo.toml file and return a CalorimeterConfig.
 *
 * Uses toml++ for parsing. Unknown top-level keys are reported via stderr
 * (helpful when a stale or mistyped key would otherwise be silently
 * ignored), but do not cause the parse to fail.
 *
 * @throws std::runtime_error if the file cannot be opened, contains
 *         malformed TOML, or is missing the mandatory `layers` field.
 */
CalorimeterConfig readCaloConfig(const std::string& path);

}  // namespace SHiPGeometry
