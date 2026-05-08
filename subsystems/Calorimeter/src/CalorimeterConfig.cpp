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

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <toml++/toml.hpp>

namespace SHiPGeometry {

namespace {

// Recognised top-level keys. Anything outside this set triggers a warning.
const std::set<std::string> kKnownKeys = {
    "layers",
    "layers2",
    "plate_xy_mm",
    "lead_thickness_mm",
    "scint_thickness_mm",
    "hpl_thickness_mm",
    "fiber_diameter_mm",
    "fiber_core_diameter_mm",
    "airgap_mm",
    "iron_thickness_mm",
    "gap_ecal_hcal_mm",
    "module_nx",
    "module_ny",
    "module_pitch_x_mm",
    "module_pitch_y_mm",
    "tol_x_mm",
    "tol_y_mm",
    "tol_z_mm",
    "detector_offset_x_mm",
    "detector_offset_y_mm",
    "detector_offset_z_mm",
    "center_stack",
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
            // strip whitespace
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
        const std::string key{k};
        if (!kKnownKeys.count(key)) {
            std::cerr << "CalorimeterConfig: warning: unknown key '" << key << "' in " << path
                      << " (typo? stale field? — value will be ignored)\n";
        }
    }

    // Second pass: read each known key if present.
    if (auto n = table["layers"]; n)
        cfg.layers = readIntList(n, "layers");
    if (auto n = table["layers2"]; n)
        cfg.layers2 = readIntList(n, "layers2");

    if (auto n = table["plate_xy_mm"]; n)
        cfg.plate_xy_mm = readNumeric(n, "plate_xy_mm");
    if (auto n = table["lead_thickness_mm"]; n)
        cfg.lead_thickness_mm = readNumeric(n, "lead_thickness_mm");
    if (auto n = table["scint_thickness_mm"]; n)
        cfg.scint_thickness_mm = readNumeric(n, "scint_thickness_mm");
    if (auto n = table["hpl_thickness_mm"]; n)
        cfg.hpl_thickness_mm = readNumeric(n, "hpl_thickness_mm");
    if (auto n = table["fiber_diameter_mm"]; n)
        cfg.fiber_diameter_mm = readNumeric(n, "fiber_diameter_mm");
    if (auto n = table["fiber_core_diameter_mm"]; n)
        cfg.fiber_core_diameter_mm = readNumeric(n, "fiber_core_diameter_mm");
    if (auto n = table["airgap_mm"]; n)
        cfg.airgap_mm = readNumeric(n, "airgap_mm");
    if (auto n = table["iron_thickness_mm"]; n)
        cfg.iron_thickness_mm = readNumeric(n, "iron_thickness_mm");
    if (auto n = table["gap_ecal_hcal_mm"]; n)
        cfg.gap_ecal_hcal_mm = readNumeric(n, "gap_ecal_hcal_mm");

    if (auto n = table["module_nx"]; n) {
        auto i = n.value<int64_t>();
        if (!i || *i <= 0 || *i > std::numeric_limits<int>::max())
            throw std::runtime_error("CalorimeterConfig: 'module_nx' must be a positive integer");
        cfg.module_nx = static_cast<int>(*i);
    }
    if (auto n = table["module_ny"]; n) {
        auto i = n.value<int64_t>();
        if (!i || *i <= 0 || *i > std::numeric_limits<int>::max())
            throw std::runtime_error("CalorimeterConfig: 'module_ny' must be a positive integer");
        cfg.module_ny = static_cast<int>(*i);
    }

    if (auto n = table["module_pitch_x_mm"]; n)
        cfg.module_pitch_x_mm = readNumeric(n, "module_pitch_x_mm");
    if (auto n = table["module_pitch_y_mm"]; n)
        cfg.module_pitch_y_mm = readNumeric(n, "module_pitch_y_mm");

    if (auto n = table["tol_x_mm"]; n)
        cfg.tol_x_mm = readNumeric(n, "tol_x_mm");
    if (auto n = table["tol_y_mm"]; n)
        cfg.tol_y_mm = readNumeric(n, "tol_y_mm");
    if (auto n = table["tol_z_mm"]; n)
        cfg.tol_z_mm = readNumeric(n, "tol_z_mm");

    if (auto n = table["detector_offset_x_mm"]; n)
        cfg.detector_offset_x_mm = readNumeric(n, "detector_offset_x_mm");
    if (auto n = table["detector_offset_y_mm"]; n)
        cfg.detector_offset_y_mm = readNumeric(n, "detector_offset_y_mm");
    if (auto n = table["detector_offset_z_mm"]; n)
        cfg.detector_offset_z_mm = readNumeric(n, "detector_offset_z_mm");

    if (auto n = table["center_stack"]; n) {
        if (auto b = n.value<bool>(); b) {
            cfg.center_stack = *b;
        } else if (auto i = n.value<int64_t>(); i) {
            cfg.center_stack = (*i != 0);
        } else if (auto s = n.value<std::string>(); s) {
            const std::string& v = *s;
            cfg.center_stack = (v == "1" || v == "true" || v == "yes" || v == "on");
        }
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
