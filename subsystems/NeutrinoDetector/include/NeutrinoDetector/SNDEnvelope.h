// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <array>
#include <string>

namespace SHiPGeometry {

/**
 * @brief The SND reservation envelope: a box subtracted from the muon-shield
 *        iron to make room for the neutrino detector.
 *
 * All values are in world/beamline coordinates (target-front-face origin).
 * Populated by readSNDEnvelope() from SD.toml.
 */
struct SNDEnvelope {
    std::array<double, 3> centre_mm = {0.0, 0.0, 28950.0};   ///< world centre (mm)
    std::array<double, 3> size_mm = {800.0, 800.0, 5100.0};  ///< full x, y, z (mm)
    std::array<double, 3> rotation_deg = {0.0, 0.0, 0.0};    ///< extrinsic X->Y->Z (deg)
};

/**
 * @brief Parse an SD.toml file into an SNDEnvelope.
 *
 * The path is resolved with the shared resolveConfigPath() fallback chain
 * (CWD, then the source tree, then the installed data dir), so callers can
 * simply use the default.
 *
 * @throws std::runtime_error on malformed TOML, a wrong-length array, or a
 *         non-positive size.
 */
SNDEnvelope readSNDEnvelope(const std::string& path = "SD.toml");

/// Resolve the SD.toml path that readSNDEnvelope() would actually open.
std::string resolvedSNDConfigPath(const std::string& path = "SD.toml");

}  // namespace SHiPGeometry
