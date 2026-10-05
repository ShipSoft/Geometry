// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

// Small shared helpers for the subsystem TOML config parsers (calorimeter,
// muon shield, neutrino detector). Factors out the unknown-key warning, the
// numeric conversion and the fixed-length numeric-array parsing that were
// duplicated across parsers. Each caller passes its own error prefix (e.g.
// "MuonShieldConfig") so messages stay self-identifying.

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
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

// Warn on stderr about every top-level key of @p table that is not in
// @p knownKeys (typos, stale fields). Unknown keys are ignored, not an error.
inline void warnUnknownKeys(const toml::table& table, std::span<const std::string_view> knownKeys,
                            const std::string& path, const char* who) {
    for (const auto& [k, _] : table) {
        if (std::ranges::find(knownKeys, std::string_view{k}) == knownKeys.end()) {
            std::cerr << who << ": warning: unknown key '" << k << "' in " << path
                      << " (typo? stale field? — value will be ignored)\n";
        }
    }
}

// Read a scalar TOML numeric (integer or float) as a double. Throws (with the
// caller's @p who prefix) if the value is not a number.
inline double readNumeric(const toml::node_view<toml::node>& node, std::string_view key,
                          const char* who) {
    if (auto v = asDouble(node.node()))
        return *v;
    throw std::runtime_error(std::string(who) + ": '" + std::string(key) + "' must be a number");
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
