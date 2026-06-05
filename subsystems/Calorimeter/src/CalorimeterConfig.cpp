// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Calorimeter configuration parser. Reads a calo.toml file and populates
// a CalorimeterConfig struct. Built on toml++
// (https://github.com/marzer/tomlplusplus, single-header MIT, fetched via
// CMake FetchContent), so the file format is standard TOML — no bespoke
// scanner, no semicolon quirks.
//
// Behaviour notes
// ===============
// * Unknown top-level keys are NOT silently ignored. Each one is reported
//   on stderr at parse time. This catches typos and stale keys that the
//   previous bespoke parser would have swallowed (this was a code-review
//   request on PR #11).
//
// * Both array-style (TOML native) and the legacy "1,2,3" comma-string
//   form are accepted for `layers` and `layers2`. The legacy form is
//   read by treating the value as a string and splitting on commas.
//   This avoids a flag day where every existing calo.toml file out there
//   has to be rewritten.
//
// * The previously-defined `pvt_thickness_mm` key is dropped: it duplicated
//   `scint_thickness_mm` and was never read by the factory. Files that
//   still mention it will trigger the unknown-key warning.

#include "Calorimeter/CalorimeterConfig.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <toml++/toml.h>

namespace SHiPGeometry {

namespace {

using namespace std::string_view_literals;

// Recognised top-level keys (sorted for binary search). Anything outside this
// set triggers a warning.
static constexpr std::array kKnownKeys = {
    "airgap_mm"sv,
    "center_stack"sv,
    "detector_offset_x_mm"sv,
    "detector_offset_y_mm"sv,
    "detector_offset_z_mm"sv,
    "fiber_core_diameter_mm"sv,
    "fiber_diameter_mm"sv,
    "gap_ecal_hcal_mm"sv,
    "hpl_thickness_mm"sv,
    "iron_thickness_mm"sv,
    "layers"sv,
    "layers2"sv,
    "lead_thickness_mm"sv,
    "module_nx"sv,
    "module_ny"sv,
    "module_pitch_x_mm"sv,
    "module_pitch_y_mm"sv,
    "plate_xy_mm"sv,
    "scint_thickness_mm"sv,
    "tol_x_mm"sv,
    "tol_y_mm"sv,
    "tol_z_mm"sv,
};

// Mapping from TOML key name to CalorimeterConfig double member pointer.
struct NumericField {
    const char* key;
    double CalorimeterConfig::* member;
};

static constexpr NumericField kNumericFields[] = {
    {"plate_xy_mm", &CalorimeterConfig::plate_xy_mm},
    {"lead_thickness_mm", &CalorimeterConfig::lead_thickness_mm},
    {"scint_thickness_mm", &CalorimeterConfig::scint_thickness_mm},
    {"hpl_thickness_mm", &CalorimeterConfig::hpl_thickness_mm},
    {"fiber_diameter_mm", &CalorimeterConfig::fiber_diameter_mm},
    {"fiber_core_diameter_mm", &CalorimeterConfig::fiber_core_diameter_mm},
    {"airgap_mm", &CalorimeterConfig::airgap_mm},
    {"iron_thickness_mm", &CalorimeterConfig::iron_thickness_mm},
    {"gap_ecal_hcal_mm", &CalorimeterConfig::gap_ecal_hcal_mm},
    {"module_pitch_x_mm", &CalorimeterConfig::module_pitch_x_mm},
    {"module_pitch_y_mm", &CalorimeterConfig::module_pitch_y_mm},
    {"tol_x_mm", &CalorimeterConfig::tol_x_mm},
    {"tol_y_mm", &CalorimeterConfig::tol_y_mm},
    {"tol_z_mm", &CalorimeterConfig::tol_z_mm},
    {"detector_offset_x_mm", &CalorimeterConfig::detector_offset_x_mm},
    {"detector_offset_y_mm", &CalorimeterConfig::detector_offset_y_mm},
    {"detector_offset_z_mm", &CalorimeterConfig::detector_offset_z_mm},
};

// Read an integer-list value: either a TOML array of ints, or a string
// of comma-separated ints (legacy form).
std::vector<int> readIntList(const toml::node_view<toml::node>& node, const std::string& key) {
    std::vector<int> out;
    if (node.is_array()) {
        for (const auto& v : *node.as_array()) {
            if (auto i = v.value<int64_t>(); i)
                out.push_back(static_cast<int>(*i));
            else
                throw std::runtime_error("CalorimeterConfig: '" + key +
                                         "' contains a non-integer value");
        }
    } else if (auto s = node.value<std::string>(); s) {
        std::stringstream ss(*s);
        std::string token;
        while (std::getline(ss, token, ',')) {
            auto first = token.find_first_not_of(" \t\r\n");
            auto last = token.find_last_not_of(" \t\r\n;");
            if (first == std::string::npos)
                continue;
            out.push_back(std::stoi(token.substr(first, last - first + 1)));
        }
    } else {
        throw std::runtime_error("CalorimeterConfig: '" + key + "' must be an array of integers");
    }
    return out;
}

// Read a double or integer as a double — TOML distinguishes them at the
// type level, but we treat the calorimeter parameters uniformly.
double readNumeric(const toml::node_view<toml::node>& node, const std::string& key) {
    if (auto d = node.value<double>(); d)
        return *d;
    if (auto i = node.value<int64_t>(); i)
        return static_cast<double>(*i);
    throw std::runtime_error("CalorimeterConfig: '" + key + "' must be a number");
}

// Read a positive integer from TOML, validating range.
int readPositiveInt(const toml::node_view<toml::node>& node, const char* key) {
    auto i = node.value<int64_t>();
    if (!i || *i <= 0 || *i > std::numeric_limits<int>::max())
        throw std::runtime_error(std::string("CalorimeterConfig: '") + key +
                                 "' must be a positive integer");
    return static_cast<int>(*i);
}

}  // namespace

CalorimeterConfig readCaloConfig(const std::string& path) {
    CalorimeterConfig cfg;

    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error("CalorimeterConfig: failed to parse " + path + ": " +
                                 std::string(e.description()));
    }

    // First pass: warn about unknown keys.
    for (const auto& [k, _] : table) {
        if (!std::ranges::binary_search(kKnownKeys, std::string_view{k})) {
            std::cerr << "CalorimeterConfig: warning: unknown key '" << k << "' in " << path
                      << " (typo? stale field? — value will be ignored)\n";
        }
    }

    // Read layer sequences.
    if (auto n = table["layers"]; n)
        cfg.layers = readIntList(n, "layers");
    if (auto n = table["layers2"]; n)
        cfg.layers2 = readIntList(n, "layers2");

    // Read all numeric (double) fields via pointer-to-member table.
    for (const auto& [key, member] : kNumericFields)
        if (auto n = table[key]; n)
            cfg.*member = readNumeric(n, key);

    // Integer fields requiring positive-value validation.
    if (auto n = table["module_nx"]; n)
        cfg.module_nx = readPositiveInt(n, "module_nx");
    if (auto n = table["module_ny"]; n)
        cfg.module_ny = readPositiveInt(n, "module_ny");

    // Boolean field — TOML native bool or integer fallback.
    if (auto n = table["center_stack"]; n) {
        if (auto b = n.value<bool>(); b)
            cfg.center_stack = *b;
        else if (auto i = n.value<int64_t>(); i)
            cfg.center_stack = (*i != 0);
    }

    if (cfg.layers.empty())
        throw std::runtime_error("CalorimeterConfig: 'layers' must be defined in " + path);

    // Default fibre-core diameter to outer diameter when not set
    if (cfg.fiber_core_diameter_mm <= 0)
        cfg.fiber_core_diameter_mm = cfg.fiber_diameter_mm;

    if (cfg.fiber_core_diameter_mm > cfg.fiber_diameter_mm)
        throw std::runtime_error(
            "CalorimeterConfig: fiber_core_diameter_mm cannot exceed fiber_diameter_mm");

    return cfg;
}

}  // namespace SHiPGeometry
