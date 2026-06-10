// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// SBT configuration parser. Reads an sbt.toml file and populates an
// SBTConfig struct, built on toml++ (standard TOML). Mirrors the
// CalorimeterConfig parser: numeric fields are read through a
// pointer-to-member table, and unknown top-level keys are reported on
// stderr rather than silently ignored.

#include "DecayVolume/SBTConfig.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <toml++/toml.h>

namespace SHiPGeometry {

namespace {

using namespace std::string_view_literals;

// Recognised top-level keys (sorted for binary search).
static constexpr std::array kKnownKeys = {
    "container_thickness_mm"sv,
    "hbeam_flange_thickness_mm"sv,
    "hbeam_flange_width_mm"sv,
    "hbeam_height_mm"sv,
    "hbeam_web_thickness_mm"sv,
    "helium_clearance_mm"sv,
    "material_air"sv,
    "material_cell"sv,
    "material_helium"sv,
    "material_steel"sv,
    "material_wall"sv,
    "n_cells"sv,
    "n_sub_frustum"sv,
    "sensor_clearance_mm"sv,
    "total_length_mm"sv,
    "wall_thickness_mm"sv,
    "x_half_entrance_mm"sv,
    "x_half_exit_mm"sv,
    "y_floor_mm"sv,
    "y_half_entrance_mm"sv,
    "y_half_exit_mm"sv,
    "z_entrance_mm"sv,
};

// Mapping from TOML key name to SBTConfig double member pointer.
struct NumericField {
    const char* key;
    double SBTConfig::* member;
};

static constexpr NumericField kNumericFields[] = {
    {"x_half_entrance_mm", &SBTConfig::x_half_entrance_mm},
    {"y_half_entrance_mm", &SBTConfig::y_half_entrance_mm},
    {"x_half_exit_mm", &SBTConfig::x_half_exit_mm},
    {"y_half_exit_mm", &SBTConfig::y_half_exit_mm},
    {"total_length_mm", &SBTConfig::total_length_mm},
    {"y_floor_mm", &SBTConfig::y_floor_mm},
    {"z_entrance_mm", &SBTConfig::z_entrance_mm},
    {"hbeam_height_mm", &SBTConfig::hbeam_height_mm},
    {"hbeam_flange_width_mm", &SBTConfig::hbeam_flange_width_mm},
    {"hbeam_flange_thickness_mm", &SBTConfig::hbeam_flange_thickness_mm},
    {"hbeam_web_thickness_mm", &SBTConfig::hbeam_web_thickness_mm},
    {"container_thickness_mm", &SBTConfig::container_thickness_mm},
    {"wall_thickness_mm", &SBTConfig::wall_thickness_mm},
    {"sensor_clearance_mm", &SBTConfig::sensor_clearance_mm},
    {"helium_clearance_mm", &SBTConfig::helium_clearance_mm},
};

// Mapping from TOML key name to SBTConfig string member pointer.
struct StringField {
    const char* key;
    std::string SBTConfig::* member;
};

static constexpr StringField kStringFields[] = {
    {"material_steel", &SBTConfig::material_steel},
    {"material_wall", &SBTConfig::material_wall},
    {"material_cell", &SBTConfig::material_cell},
    {"material_helium", &SBTConfig::material_helium},
    {"material_air", &SBTConfig::material_air},
};

double readNumeric(const toml::node_view<toml::node>& node, const char* key) {
    if (auto d = node.value<double>(); d)
        return *d;
    if (auto i = node.value<int64_t>(); i)
        return static_cast<double>(*i);
    throw std::runtime_error(std::string("SBTConfig: '") + key + "' must be a number");
}

std::string readString(const toml::node_view<toml::node>& node, const char* key) {
    if (auto s = node.value<std::string>(); s)
        return *s;
    throw std::runtime_error(std::string("SBTConfig: '") + key + "' must be a string");
}

int readPositiveInt(const toml::node_view<toml::node>& node, const char* key) {
    auto i = node.value<int64_t>();
    if (!i || *i <= 0 || *i > std::numeric_limits<int>::max())
        throw std::runtime_error(std::string("SBTConfig: '") + key +
                                 "' must be a positive integer");
    return static_cast<int>(*i);
}

}  // namespace

SBTConfig readSBTConfig(const std::string& path) {
    SBTConfig cfg;

    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error("SBTConfig: failed to parse " + path + ": " +
                                 std::string(e.description()));
    }

    // Warn about unknown keys.
    for (const auto& [k, _] : table) {
        if (!std::ranges::binary_search(kKnownKeys, std::string_view{k})) {
            std::cerr << "SBTConfig: warning: unknown key '" << k << "' in " << path
                      << " (typo? stale field? — value will be ignored)\n";
        }
    }

    // Numeric (double) fields via pointer-to-member table.
    for (const auto& [key, member] : kNumericFields)
        if (auto n = table[key]; n)
            cfg.*member = readNumeric(n, key);

    // String fields (material names).
    for (const auto& [key, member] : kStringFields)
        if (auto n = table[key]; n)
            cfg.*member = readString(n, key);

    // Positive-integer fields.
    if (auto n = table["n_sub_frustum"]; n)
        cfg.n_sub_frustum = readPositiveInt(n, "n_sub_frustum");
    if (auto n = table["n_cells"]; n)
        cfg.n_cells = readPositiveInt(n, "n_cells");

    // Domain checks: catch geometrically invalid configs here so downstream
    // builders cannot hit a division-by-zero or build a negative-size shape.
    // (n_sub_frustum and n_cells are already guaranteed positive above.)
    auto requirePositive = [](double value, const char* key) {
        if (value <= 0.0)
            throw std::runtime_error(std::string("SBTConfig: '") + key + "' must be > 0");
    };
    requirePositive(cfg.total_length_mm, "total_length_mm");  // denominator in x/yHalfAtZ
    requirePositive(cfg.x_half_entrance_mm, "x_half_entrance_mm");
    requirePositive(cfg.y_half_entrance_mm, "y_half_entrance_mm");
    requirePositive(cfg.x_half_exit_mm, "x_half_exit_mm");
    requirePositive(cfg.y_half_exit_mm, "y_half_exit_mm");
    requirePositive(cfg.hbeam_flange_width_mm, "hbeam_flange_width_mm");
    requirePositive(cfg.hbeam_flange_thickness_mm, "hbeam_flange_thickness_mm");
    requirePositive(cfg.hbeam_web_thickness_mm, "hbeam_web_thickness_mm");
    requirePositive(cfg.container_thickness_mm, "container_thickness_mm");
    requirePositive(cfg.wall_thickness_mm, "wall_thickness_mm");
    // Clearances are insets/gaps; a non-positive value collapses the gap and
    // can overlap geometry (helium_clearance_mm feeds the helium inset =
    // container_thickness_mm + helium_clearance_mm in DecayVolumeFactory).
    requirePositive(cfg.sensor_clearance_mm, "sensor_clearance_mm");
    requirePositive(cfg.helium_clearance_mm, "helium_clearance_mm");
    // Web half-height (= height/2 - flange_thickness) is used as a GeoBox
    // half-dimension, so the flanges must not consume the whole beam.
    if (cfg.hbeam_height_mm <= 2.0 * cfg.hbeam_flange_thickness_mm)
        throw std::runtime_error(
            "SBTConfig: 'hbeam_height_mm' must exceed 2 * 'hbeam_flange_thickness_mm' "
            "(otherwise the H-beam web height is non-positive)");

    return cfg;
}

}  // namespace SHiPGeometry
