// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

// Small shared helpers for the subsystem TOML config parsers (calorimeter,
// muon shield, neutrino detector). Factors out the numeric conversion and the
// fixed-length numeric-array parsing that were duplicated across parsers. Each
// caller passes its own error prefix (e.g. "MuonShieldConfig") so messages stay
// self-identifying.

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <toml++/toml.h>

namespace SHiPGeometry::tomlconfig {

// Extract a TOML numeric (integer or float) as a double, if the node holds one.
inline std::optional<double> asDouble(const toml::node* node) {
    if (node) {
        if (auto d = node->value<double>())
            return *d;
        if (auto i = node->value<int64_t>())
            return static_cast<double>(*i);
    }
    return std::nullopt;
}

// Read a fixed-length numeric array (e.g. size = [x, y, z]) from a table.
// Accepts TOML integers or floats. Throws (with the caller's @p who prefix) on
// a missing required key, a wrong-length array, or a non-numeric element.
template <std::size_t N>
std::array<double, N> readNumericArray(const toml::table& table, const char* key,
                                       const std::string& path, bool required,
                                       const std::array<double, N>& fallback, const char* who) {
    auto node = table[key];
    if (!node) {
        if (required)
            throw std::runtime_error(std::string(who) + ": missing required '" + std::string(key) +
                                     "' in " + path);
        return fallback;
    }
    const toml::array* arr = node.as_array();
    if (!arr || arr->size() != N)
        throw std::runtime_error(std::string(who) + ": '" + std::string(key) + "' must be a " +
                                 std::to_string(N) + "-element array in " + path);
    std::array<double, N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        if (auto v = asDouble(arr->get(i)))
            out[i] = *v;
        else
            throw std::runtime_error(std::string(who) + ": '" + std::string(key) +
                                     "' values must be numbers in " + path);
    }
    return out;
}

}  // namespace SHiPGeometry::tomlconfig
