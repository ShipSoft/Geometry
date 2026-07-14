// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <cmath>
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
    // Gap left between the helium and the innermost SBT material, measured along
    // the coordinate axis. Must be >= 0; SBTEnvelope enforces that and is the
    // only consumer. It is not a safety margin — the default of 1 um is simply
    // enough to stop the helium and the SBT sharing a surface, which Geant4's
    // navigator handles badly. 0 is legal and gives exact face-to-face contact.
    // NOTE: this default must track sbt.toml.
    double helium_clearance_mm = 0.001;

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

    // ── Frustum profile ─────────────────────────────────────────────────
    /// Exit-face Z in the DecayVolume local frame (mm).
    double zExit() const { return z_entrance_mm + total_length_mm; }
    /// dx_half/dz of the frustum (dimensionless).
    double xGrowth() const { return (x_half_exit_mm - x_half_entrance_mm) / total_length_mm; }
    /// dy_half/dz of the frustum (dimensionless).
    double yGrowth() const { return (y_half_exit_mm - y_half_entrance_mm) / total_length_mm; }
    /// Half-extent of the frustum envelope in X at a given Z (mm).
    double xHalfAt(double z_mm) const {
        return x_half_entrance_mm + (z_mm - z_entrance_mm) * xGrowth();
    }
    /// Half-extent of the frustum envelope in Y at a given Z (mm).
    double yHalfAt(double z_mm) const {
        return y_half_entrance_mm + (z_mm - z_entrance_mm) * yGrowth();
    }
    /// Z of the start of sub-frustum @p s (mm).
    double zSubLo(int s) const { return z_entrance_mm + s * subLength(); }

    // ── H-beam cross-section primitives ─────────────────────────────────
    /// Offset of a flange's mid-plane from the beam axis (mm).
    double hbeamFlangeOffset() const {
        return 0.5 * hbeam_height_mm - 0.5 * hbeam_flange_thickness_mm;
    }
    /// Reach of a flange's *outer* surface from the beam axis (mm) = h/2.
    double hbeamHalfHeight() const { return 0.5 * hbeam_height_mm; }

    // ══════════════════════════════════════════════════════════════════════
    //  PLACEMENT PRIMITIVES — the single source of truth.
    //
    //  These are THE definitions of where SBT material sits. Both builders
    //  place volumes with them, and SBTEnvelope derives the helium inner
    //  envelope from them, so a change to any rule below propagates to the
    //  helium automatically and the two can never drift apart.
    //  Do not open-code these expressions anywhere else.
    // ══════════════════════════════════════════════════════════════════════

    /// Z offset, from the start of a sub-frustum, of the column front-flange
    /// outer edge. Sensor containers are split here: the piece upstream of it
    /// must present a *flat* outer face, or it would eat into the column.
    double zSplitOffset() const { return 0.5 * hbeam_height_mm + 0.5 * hbeam_flange_thickness_mm; }

    // --- Side (±X) scintillator containers -------------------------------
    /// Half-thickness of a side container in X (mm).
    double sideContainerHalfThickness() const { return 0.5 * container_thickness_mm; }
    /// |X| of a side container's centroid, given the local frustum half-width.
    double sideContainerCentreX(double x_half_mm) const {
        return x_half_mm - 0.5 * hbeam_flange_width_mm - sideContainerHalfThickness();
    }
    /// |X| of a side container's innermost face (mm).
    double sideSensorInnerX(double x_half_mm) const {
        return sideContainerCentreX(x_half_mm) - sideContainerHalfThickness();
    }

    // --- Top/bottom (±Y) scintillator containers -------------------------
    /// Half-thickness of a top/bottom container in Y (mm).
    double topBottomContainerHalfThickness() const {
        return 0.5 * container_thickness_mm - sensor_clearance_mm;
    }
    /// |Y| of a top/bottom container's centroid (mm).
    double topBottomContainerCentreY(double y_half_mm) const {
        return y_half_mm - 0.5 * container_thickness_mm;
    }
    /// |Y| of a top/bottom container's innermost face (mm).
    double topBottomSensorInnerY(double y_half_mm) const {
        return topBottomContainerCentreY(y_half_mm) - topBottomContainerHalfThickness();
    }
    /// Half-extent in X available to top/bottom containers (mm).
    ///
    /// The container stops half a flange width short of the frustum wall (that
    /// is where the columns' inner face is), less a 1 mm gap so it does not
    /// land flush against them. The 1 mm is inherited from the original
    /// SBTSensorBuilder and is a literal, not sensor_clearance_mm — the two
    /// happen to be equal at the current settings, which is worth being aware
    /// of when changing sensor_clearance_mm, but they are not the same quantity
    /// and this one is deliberately left as-is.
    double topBottomAvailX(double x_half_mm) const {
        return x_half_mm - 0.5 * hbeam_flange_width_mm - 1.0;
    }

    // --- Top/bottom longitudinal beams -----------------------------------
    //  NOTE: these beams *straddle* the scintillator plane — the outer flange
    //  sits above it, the web is omitted (it would pass through the cells),
    //  and the INNER FLANGE HANGS BELOW IT, INSIDE THE DECAY REGION. It, not
    //  the scintillator, is the innermost material in ±Y.
    /// |Y| of a top/bottom longitudinal beam's axis (mm).
    double longBeamCentreY(double y_half_mm) const { return y_half_mm - 0.5 * webHeight(); }
    /// |Y| reached by a longitudinal beam's inner flange surface (mm).
    ///
    /// The beam is inclined by the frustum taper, so its cross-section is
    /// rotated: the flange surface lies hbeamHalfHeight()/cos(atan(yGrowth))
    /// from the axis measured in world Y, not hbeamHalfHeight().
    double longBeamInnerY(double y_half_mm) const {
        const double g = yGrowth();
        return longBeamCentreY(y_half_mm) - hbeamHalfHeight() * std::sqrt(1.0 + g * g);
    }
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
