// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Muon shield configuration parser. Reads an MS.toml file and populates a
// MuonShieldConfig struct. Built on toml++
// (https://github.com/marzer/tomlplusplus, single-header MIT, provided by the
// tomlplusplus package), so the file format is standard TOML.
//
// Behaviour notes
// ===============
// * Unknown top-level keys are NOT silently ignored. Each one is reported on
//   stderr at parse time. This catches typos and stale keys.
//
// * Numeric fields accept both TOML integers and floats.

#include "MuonShield/MuonShieldConfig.h"

#include "SHiPGeometry/TomlConfig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <toml++/toml.h>

namespace SHiPGeometry {

namespace {

using namespace std::string_view_literals;

// Recognised top-level keys (sorted for binary search). Anything outside this
// set triggers a warning. "block" is the [[block]] array of tables.
static constexpr std::array kKnownKeys = {
    "block"sv,
    "block_material"sv,
    "envelope_half_x_mm"sv,
    "envelope_half_y_mm"sv,
    "envelope_z_end_m"sv,
    "envelope_z_start_m"sv,
};

// Mapping from TOML key name to MuonShieldConfig double member pointer.
struct NumericField {
    const char* key;
    double MuonShieldConfig::* member;
};

static constexpr NumericField kNumericFields[] = {
    {"envelope_half_x_mm", &MuonShieldConfig::envelope_half_x_mm},
    {"envelope_half_y_mm", &MuonShieldConfig::envelope_half_y_mm},
    {"envelope_z_start_m", &MuonShieldConfig::envelope_z_start_m},
    {"envelope_z_end_m", &MuonShieldConfig::envelope_z_end_m},
};

// Read a scalar double or integer as a double (shared numeric extraction).
double readNumeric(const toml::node_view<toml::node>& node, const std::string& key) {
    if (auto v = tomlconfig::asDouble(node.node()))
        return *v;
    throw std::runtime_error("MuonShieldConfig: '" + key + "' must be a number");
}

}  // namespace

MuonShieldConfig readMuonShieldConfig(const std::string& path) {
    MuonShieldConfig cfg;

    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error("MuonShieldConfig: failed to parse " + path + ": " +
                                 std::string(e.description()));
    }

    // First pass: warn about unknown keys.
    for (const auto& [k, _] : table) {
        if (!std::ranges::binary_search(kKnownKeys, std::string_view{k})) {
            std::cerr << "MuonShieldConfig: warning: unknown key '" << k << "' in " << path
                      << " (typo? stale field? — value will be ignored)\n";
        }
    }

    // Numeric (double) envelope fields.
    for (const auto& [key, member] : kNumericFields)
        if (auto n = table[key]; n)
            cfg.*member = readNumeric(n, key);

    // String field.
    if (auto n = table["block_material"]; n) {
        if (auto s = n.value<std::string>(); s)
            cfg.block_material = *s;
        else
            throw std::runtime_error("MuonShieldConfig: 'block_material' must be a string");
    }

    // Blocks: an array of tables ([[block]]).
    if (auto blocksNode = table["block"]; blocksNode) {
        const toml::array* arr = blocksNode.as_array();
        if (!arr)
            throw std::runtime_error("MuonShieldConfig: 'block' must be an array of tables in " +
                                     path);
        for (const auto& elem : *arr) {
            const toml::table* bt = elem.as_table();
            if (!bt)
                throw std::runtime_error(
                    "MuonShieldConfig: each [[block]] entry must be a table in " + path);
            MuonShieldBlock block;
            block.start = tomlconfig::readNumericArray<3>(*bt, "start", path, true, {0.0, 0.0, 0.0},
                                                          "MuonShieldConfig");
            block.size = tomlconfig::readNumericArray<3>(*bt, "size", path, true, {0.0, 0.0, 0.0},
                                                         "MuonShieldConfig");
            block.rotation_deg = tomlconfig::readNumericArray<3>(
                *bt, "rotation", path, false, {0.0, 0.0, 0.0}, "MuonShieldConfig");
            block.taper_deg = tomlconfig::readNumericArray<2>(*bt, "taper", path, false, {0.0, 0.0},
                                                              "MuonShieldConfig");
            cfg.blocks.push_back(block);
        }
    }

    // ── Validation ──────────────────────────────────────────────────────
    if (cfg.envelope_z_end_m <= cfg.envelope_z_start_m)
        throw std::runtime_error(
            "MuonShieldConfig: envelope_z_end_m must be greater than envelope_z_start_m in " +
            path);
    if (cfg.envelope_half_x_mm <= 0.0 || cfg.envelope_half_y_mm <= 0.0)
        throw std::runtime_error(
            "MuonShieldConfig: envelope_half_x_mm and envelope_half_y_mm must be positive in " +
            path);

    const double envStartMm = cfg.envelope_z_start_m * 1000.0;
    const double envEndMm = cfg.envelope_z_end_m * 1000.0;
    constexpr double kEps = 1e-6;  // mm

    for (std::size_t i = 0; i < cfg.blocks.size(); ++i) {
        const auto& b = cfg.blocks[i];
        const std::string where = "block " + std::to_string(i) + " in " + path;
        if (b.size[0] <= 0.0 || b.size[1] <= 0.0 || b.size[2] <= 0.0)
            throw std::runtime_error("MuonShieldConfig: " + where + " has a non-positive size");

        // Taper must not collapse (or invert) the downstream face.
        const double farHalfX = 0.5 * b.size[0] + b.size[2] * std::tan(b.taper_deg[0] * kDegToRad);
        const double farHalfY = 0.5 * b.size[1] + b.size[2] * std::tan(b.taper_deg[1] * kDegToRad);
        if (farHalfX <= 0.0 || farHalfY <= 0.0)
            throw std::runtime_error("MuonShieldConfig: " + where +
                                     " has a taper that collapses its downstream face");

        // The block must sit within the envelope in Z (light sanity check;
        // rotations are not accounted for here).
        if (b.start[2] < envStartMm - kEps || b.start[2] > envEndMm + kEps)
            throw std::runtime_error("MuonShieldConfig: " + where + " has its upstream face (z = " +
                                     std::to_string(b.start[2]) + " mm) outside the envelope");
        if (b.start[2] + b.size[2] > envEndMm + kEps)
            throw std::runtime_error(
                "MuonShieldConfig: " + where + " has its downstream face (z = " +
                std::to_string(b.start[2] + b.size[2]) + " mm) beyond the envelope end");
    }

    return cfg;
}

}  // namespace SHiPGeometry
