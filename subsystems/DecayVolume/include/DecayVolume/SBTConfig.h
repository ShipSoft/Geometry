// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

namespace SHiPGeometry {

/**
 * @brief Configuration for the Surround Background Tagger (SBT) geometry.
 *
 * Populated by readSBTConfig() from an sbt.toml file. Drives the steel
 * H-beam supporting structure, the LAB scintillator sensor containers, and
 * the central helium volume of the DecayVolume subsystem. All lengths in mm.
 */
struct SBTConfig {
    // ── Frustum envelope ────────────────────────────────────────────────
    double x_half_entrance_mm = 1000.0;
    double y_half_entrance_mm = 1500.0;
    double x_half_exit_mm = 2000.0;
    double y_half_exit_mm = 3000.0;
    double total_length_mm = 50000.0;
    int n_sub_frustum = 10;
    double y_floor_mm = -3000.0;
    double z_entrance_mm = -25000.0;

    // ── H-beam cross-section (HEA 260 approximation) ────────────────────
    double hbeam_height_mm = 250.0;
    double hbeam_flange_width_mm = 260.0;
    double hbeam_flange_thickness_mm = 12.5;
    double hbeam_web_thickness_mm = 7.5;

    // ── Sensor containers / cells ───────────────────────────────────────
    double container_thickness_mm = 225.0;
    double wall_thickness_mm = 5.0;
    int n_cells = 6;
    double sensor_clearance_mm = 1.0;

    // ── Helium decay region ─────────────────────────────────────────────
    double helium_clearance_mm = 10.0;

    // ── Material names (resolved from SHiPMaterials) ────────────────────
    std::string material_steel = "Iron";
    std::string material_wall = "Aluminium";
    std::string material_cell = "LAB";
    std::string material_helium = "PressurisedHe90";
    std::string material_air = "Air";

    // ── Derived quantities ──────────────────────────────────────────────
    /// Length of one sub-frustum along Z (mm).
    double subLength() const { return total_length_mm / n_sub_frustum; }
    /// Clear web height = height - 2*flange thickness (mm).
    double webHeight() const { return hbeam_height_mm - 2.0 * hbeam_flange_thickness_mm; }
    /// Number of aluminium walls per container (n_cells + 1).
    int nWalls() const { return n_cells + 1; }
};

/**
 * @brief Parse an sbt.toml file and return an SBTConfig.
 *
 * Uses toml++. Unknown top-level keys are reported on stderr but do not
 * cause the parse to fail.
 *
 * @throws std::runtime_error if the file cannot be opened or is malformed.
 */
SBTConfig readSBTConfig(const std::string& path);

}  // namespace SHiPGeometry
