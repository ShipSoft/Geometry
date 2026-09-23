// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <array>
#include <string>
#include <vector>

namespace SHiPGeometry {

/// Degrees-to-radians conversion, shared by the config parser and the factory.
inline constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

/**
 * @brief A single muon-shield iron block.
 *
 * A block is a box (or, when tapered, a symmetric trapezoid/frustum) placed by
 * its upstream (−z) face. All lengths are in millimetres and all angles in
 * degrees, in world/beamline coordinates (target-front-face origin).
 *
 * Geometry conventions:
 *  - @c start is the centre of the block's upstream face; the block extends
 *    downstream (+z) by @c size[2] before rotation.
 *  - @c rotation is applied about @c start, extrinsically about the world X, Y
 *    then Z axes.
 *  - @c size = {sx, sy, sz}: the full X and Y of the *upstream* face and the Z
 *    length.
 *  - @c taper = {ax, ay}: symmetric half-opening angles. The cross-section
 *    grows (or, if negative, shrinks) toward the downstream face, so the far
 *    half-width is (sx/2 + sz·tan(ax)) and similarly for Y. {0, 0} → a box.
 */
struct MuonShieldBlock {
    std::array<double, 3> start = {0.0, 0.0, 0.0};         ///< upstream-face centre (mm)
    std::array<double, 3> size = {0.0, 0.0, 0.0};          ///< near-face sx, sy and length sz (mm)
    std::array<double, 3> rotation_deg = {0.0, 0.0, 0.0};  ///< rotation about x, y, z (deg)
    std::array<double, 2> taper_deg = {0.0, 0.0};          ///< half-opening angles ax, ay (deg)
};

/**
 * @brief Configuration for the SHiP muon shield geometry.
 *
 * Populated by readMuonShieldConfig() from an muon_shield.toml file.
 *
 * The shield is an explicit list of iron @c blocks placed inside an Air
 * @c envelope container. The envelope sizes the container and fixes the
 * subsystem's world placement (its Z centre); the blocks are the only iron.
 * Holes/apertures are represented simply by the absence of a block, or by
 * arranging several blocks around the gap.
 */
struct MuonShieldConfig {
    // Absorber material for all blocks (must exist in SHiPMaterials).
    std::string block_material = "Iron";

    // Container (envelope) transverse half-sizes (mm).
    double envelope_half_x_mm = 1760.0;
    double envelope_half_y_mm = 1320.0;

    // Envelope Z extent in world coordinates (target-front-face origin, m).
    // This is the Air container and the world placement of the subsystem.
    double envelope_z_start_m = 4.54;
    double envelope_z_end_m = 32.08;

    // The iron blocks. Empty = no iron.
    std::vector<MuonShieldBlock> blocks;

    // ── Derived helpers ─────────────────────────────────────────────────
    /// Full Z length of the envelope (mm).
    double envelopeLengthZ_mm() const { return (envelope_z_end_m - envelope_z_start_m) * 1000.0; }
    /// World-Z centre of the envelope (mm).
    double envelopeCentreZ_mm() const {
        return 0.5 * (envelope_z_start_m + envelope_z_end_m) * 1000.0;
    }
};

/**
 * @brief Parse an muon_shield.toml file and return a MuonShieldConfig.
 *
 * Uses toml++ for parsing. Unknown top-level keys are reported via stderr
 * (helpful when a stale or mistyped key would otherwise be silently ignored),
 * but do not cause the parse to fail.
 *
 * @throws std::runtime_error if the file cannot be opened, contains malformed
 *         TOML, has a non-positive or inverted Z envelope, or a block with a
 *         non-positive size, a taper that collapses the far face, or an
 *         upstream face outside the envelope in Z.
 */
MuonShieldConfig readMuonShieldConfig(const std::string& path);

}  // namespace SHiPGeometry
