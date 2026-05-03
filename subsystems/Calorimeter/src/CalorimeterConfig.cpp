// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "Calorimeter/CalorimeterConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SHiPGeometry {

namespace {

std::string trim(std::string s) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::vector<int> parseIntList(const std::string& s) {
    std::vector<int> out;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trim(token);
        // strip trailing semicolons (calo.cfg has e.g. "gap_ecal_hcal_mm = 100;")
        if (!token.empty() && token.back() == ';')
            token.pop_back();
        if (!token.empty())
            out.push_back(std::stoi(token));
    }
    return out;
}

double toDouble(std::string v) {
    // strip optional trailing semicolon
    if (!v.empty() && v.back() == ';')
        v.pop_back();
    return std::stod(v);
}

int toInt(std::string v) {
    if (!v.empty() && v.back() == ';')
        v.pop_back();
    return std::stoi(v);
}

}  // namespace

CalorimeterConfig readCaloConfig(const std::string& path) {
    CalorimeterConfig cfg;

    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("CalorimeterConfig: cannot open: " + path);

    std::string line;
    while (std::getline(in, line)) {
        // strip comments
        auto hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);
        line = trim(line);
        if (line.empty())
            continue;

        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        auto key = trim(line.substr(0, eq));
        auto val = trim(line.substr(eq + 1));

        if (key == "layers")
            cfg.layers = parseIntList(val);
        else if (key == "layers2")
            cfg.layers2 = parseIntList(val);
        else if (key == "plate_xy_mm")
            cfg.plate_xy_mm = toDouble(val);
        else if (key == "lead_thickness_mm")
            cfg.lead_thickness_mm = toDouble(val);
        else if (key == "scint_thickness_mm")
            cfg.scint_thickness_mm = toDouble(val);
        else if (key == "pvt_thickness_mm")
            cfg.pvt_thickness_mm = toDouble(val);
        else if (key == "hpl_thickness_mm")
            cfg.hpl_thickness_mm = toDouble(val);
        else if (key == "fiber_diameter_mm")
            cfg.fiber_diameter_mm = toDouble(val);
        else if (key == "fiber_core_diameter_mm")
            cfg.fiber_core_diameter_mm = toDouble(val);
        else if (key == "airgap_mm")
            cfg.airgap_mm = toDouble(val);
        else if (key == "iron_thickness_mm")
            cfg.iron_thickness_mm = toDouble(val);
        else if (key == "gap_ecal_hcal_mm")
            cfg.gap_ecal_hcal_mm = toDouble(val);
        else if (key == "module_nx")
            cfg.module_nx = toInt(val);
        else if (key == "module_ny")
            cfg.module_ny = toInt(val);
        else if (key == "module_pitch_x_mm")
            cfg.module_pitch_x_mm = toDouble(val);
        else if (key == "module_pitch_y_mm")
            cfg.module_pitch_y_mm = toDouble(val);
        else if (key == "tol_x_mm")
            cfg.tol_x_mm = toDouble(val);
        else if (key == "tol_y_mm")
            cfg.tol_y_mm = toDouble(val);
        else if (key == "tol_z_mm")
            cfg.tol_z_mm = toDouble(val);
        else if (key == "detector_offset_x_mm")
            cfg.detector_offset_x_mm = toDouble(val);
        else if (key == "detector_offset_y_mm")
            cfg.detector_offset_y_mm = toDouble(val);
        else if (key == "detector_offset_z_mm")
            cfg.detector_offset_z_mm = toDouble(val);
        else if (key == "center_stack") {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            cfg.center_stack = (v == "1" || v == "true" || v == "yes" || v == "on");
        }
        // unknown keys are silently ignored to allow forward-compatibility
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
