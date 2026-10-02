// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "NeutrinoDetector/SNDEnvelope.h"

#include "SHiPGeometry/ConfigPath.h"
#include "SHiPGeometry/TomlConfig.h"

#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <toml++/toml.h>

// Absolute fallback paths baked in by CMake so out-of-source builds always find
// SD.toml even when the CWD doesn't contain a copy of it.
#ifndef SD_TOML_DEFAULT_PATH
#define SD_TOML_DEFAULT_PATH "SD.toml"
#endif
#ifndef SD_TOML_INSTALL_PATH
#define SD_TOML_INSTALL_PATH ""
#endif

namespace SHiPGeometry {

namespace {

std::array<double, 3> readVec3(const toml::table& table, const char* key, const std::string& path,
                               bool required, const std::array<double, 3>& fallback) {
    return tomlconfig::readNumericArray<3>(table, key, path, required, fallback, "SNDEnvelope");
}

}  // namespace

std::string resolvedSNDConfigPath(const std::string& path) {
    return resolveConfigPath(path, SD_TOML_DEFAULT_PATH, SD_TOML_INSTALL_PATH);
}

SNDEnvelope readSNDEnvelope(const std::string& rawPath) {
    const std::string path = resolvedSNDConfigPath(rawPath);
    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error("SNDEnvelope: failed to parse " + path + ": " +
                                 std::string(e.description()));
    }

    SNDEnvelope env;

    // Warn about unknown top-level keys (typos / stale fields), like
    // readMuonShieldConfig. Only centre, size and rotation are recognised.
    for (const auto& [k, _] : table) {
        const std::string_view key{k};
        if (key != "centre" && key != "size" && key != "rotation")
            std::cerr << "SNDEnvelope: warning: unknown key '" << k << "' in " << path
                      << " (ignored)\n";
    }

    env.centre_mm = readVec3(table, "centre", path, true, env.centre_mm);
    env.size_mm = readVec3(table, "size", path, true, env.size_mm);
    env.rotation_deg = readVec3(table, "rotation", path, false, env.rotation_deg);

    if (env.size_mm[0] <= 0.0 || env.size_mm[1] <= 0.0 || env.size_mm[2] <= 0.0)
        throw std::runtime_error("SNDEnvelope: size must be positive in " + path);

    return env;
}

}  // namespace SHiPGeometry
