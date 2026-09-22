// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Muon shield configuration parser. Reads an muon_shield.toml file and populates a
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

// ── block geometry helpers ───────────────────────────────────────────────────
//
// The validation below is rotation-aware, which needs a little linear algebra.
// It is written out here with std::cos/std::sin rather than pulled in from
// GeoModel: this parser has no GeoModel dependency and is worth keeping that
// way, and the factory applies the same convention via GeoTrf.

using Matrix3 = std::array<std::array<double, 3>, 3>;  // row-major

Matrix3 multiply(const Matrix3& a, const Matrix3& b) {
    Matrix3 out{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            for (std::size_t k = 0; k < 3; ++k)
                out[i][j] += a[i][k] * b[k][j];
    return out;
}

// Extrinsic X→Y→Z rotation, matching MuonShieldFactory's
// RotateZ3D * RotateY3D * RotateX3D.
Matrix3 rotationMatrix(const std::array<double, 3>& deg) {
    const double cx = std::cos(deg[0] * kDegToRad), sx = std::sin(deg[0] * kDegToRad);
    const double cy = std::cos(deg[1] * kDegToRad), sy = std::sin(deg[1] * kDegToRad);
    const double cz = std::cos(deg[2] * kDegToRad), sz = std::sin(deg[2] * kDegToRad);
    const Matrix3 rx{{{1.0, 0.0, 0.0}, {0.0, cx, -sx}, {0.0, sx, cx}}};
    const Matrix3 ry{{{cy, 0.0, sy}, {0.0, 1.0, 0.0}, {-sy, 0.0, cy}}};
    const Matrix3 rz{{{cz, -sz, 0.0}, {sz, cz, 0.0}, {0.0, 0.0, 1.0}}};
    return multiply(rz, multiply(ry, rx));
}

/// An oriented bounding box in world coordinates.
struct Obb {
    std::array<double, 3> centre{};  ///< world centre
    std::array<double, 3> half{};    ///< half-extents along the local axes
    Matrix3 rot{};                   ///< local→world; column j is local axis j
};

double axisComponent(const Matrix3& rot, std::size_t axis, std::size_t component) {
    return rot[component][axis];  // column `axis`
}

/// Map a point given in @p o's local frame into world coordinates.
std::array<double, 3> toWorld(const Obb& o, double lx, double ly, double lz) {
    std::array<double, 3> out = o.centre;
    for (std::size_t c = 0; c < 3; ++c)
        out[c] += lx * axisComponent(o.rot, 0, c) + ly * axisComponent(o.rot, 1, c) +
                  lz * axisComponent(o.rot, 2, c);
    return out;
}

/// The bounding box of a block, in world coordinates, with its rotation.
///
/// The half-extents use the widest of the near and far faces, so a tapered
/// block (a GeoTrd) is enclosed rather than cut — conservative in the direction
/// that matters here: the overlap test may flag a near miss, but never misses a
/// real intersection.
Obb blockObb(const MuonShieldBlock& blk) {
    const double halfLenZ = 0.5 * blk.size[2];
    const double farHalfX =
        0.5 * blk.size[0] + blk.size[2] * std::tan(blk.taper_deg[0] * kDegToRad);
    const double farHalfY =
        0.5 * blk.size[1] + blk.size[2] * std::tan(blk.taper_deg[1] * kDegToRad);

    Obb o;
    o.rot = rotationMatrix(blk.rotation_deg);
    o.half = {std::max(0.5 * blk.size[0], farHalfX), std::max(0.5 * blk.size[1], farHalfY),
              halfLenZ};
    // The block is anchored by its upstream face at `start` and rotated about
    // that point, so its centre is half a length downstream along the rotated
    // local Z.
    o.centre = blk.start;
    for (std::size_t c = 0; c < 3; ++c)
        o.centre[c] += halfLenZ * axisComponent(o.rot, 2, c);
    return o;
}

/// Separating-axis test for two oriented boxes.
///
/// Returns true only when they genuinely interpenetrate. Both boxes are first
/// shrunk by @p eps/2 on every face, so blocks that merely touch come out
/// separated — cheaper and more robust than slackening each of the 15 axis
/// tests, which would misreport the degenerate cross-product axes of two
/// parallel boxes as separations.
bool obbsOverlap(const Obb& a, const Obb& b, double eps) {
    const std::array<double, 3> ha{a.half[0] - 0.5 * eps, a.half[1] - 0.5 * eps,
                                   a.half[2] - 0.5 * eps};
    const std::array<double, 3> hb{b.half[0] - 0.5 * eps, b.half[1] - 0.5 * eps,
                                   b.half[2] - 0.5 * eps};

    // r[i][j] = a's axis i dotted with b's axis j; t = centre offset in a's frame.
    double r[3][3], absR[3][3], t[3];
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) {
            r[i][j] = 0.0;
            for (std::size_t c = 0; c < 3; ++c)
                r[i][j] += axisComponent(a.rot, i, c) * axisComponent(b.rot, j, c);
            // Guards the near-parallel case, where a cross-product axis
            // degenerates to (almost) zero and its own test is meaningless.
            absR[i][j] = std::abs(r[i][j]) + 1e-12;
        }
    const std::array<double, 3> d{b.centre[0] - a.centre[0], b.centre[1] - a.centre[1],
                                  b.centre[2] - a.centre[2]};
    for (std::size_t i = 0; i < 3; ++i) {
        t[i] = 0.0;
        for (std::size_t c = 0; c < 3; ++c)
            t[i] += d[c] * axisComponent(a.rot, i, c);
    }

    // 3 axes of a, then 3 axes of b.
    for (std::size_t i = 0; i < 3; ++i) {
        const double rb = hb[0] * absR[i][0] + hb[1] * absR[i][1] + hb[2] * absR[i][2];
        if (std::abs(t[i]) > ha[i] + rb)
            return false;
    }
    for (std::size_t j = 0; j < 3; ++j) {
        const double ra = ha[0] * absR[0][j] + ha[1] * absR[1][j] + ha[2] * absR[2][j];
        const double tj = t[0] * r[0][j] + t[1] * r[1][j] + t[2] * r[2][j];
        if (std::abs(tj) > ra + hb[j])
            return false;
    }

    // The 9 cross-product axes a_i x b_j.
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) {
            const std::size_t i1 = (i + 1) % 3, i2 = (i + 2) % 3;
            const std::size_t j1 = (j + 1) % 3, j2 = (j + 2) % 3;
            const double ra = ha[i1] * absR[i2][j] + ha[i2] * absR[i1][j];
            const double rb = hb[j1] * absR[i][j2] + hb[j2] * absR[i][j1];
            if (std::abs(t[i2] * r[i1][j] - t[i1] * r[i2][j]) > ra + rb)
                return false;
        }

    return true;
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

        // The anchor itself must sit within the envelope in Z. This is about
        // where the block is placed, so it is checked on `start` directly and
        // is independent of any rotation.
        if (b.start[2] < envStartMm - kEps || b.start[2] > envEndMm + kEps)
            throw std::runtime_error("MuonShieldConfig: " + where + " has its upstream face (z = " +
                                     std::to_string(b.start[2]) + " mm) outside the envelope");

        // The block's extent must fit the envelope (clearance vs neighbouring
        // structures such as TCC8). Taken from the eight rotated corners, using
        // each face's own half-widths, so it is exact for a tapered block and
        // does not reject a rotated block that genuinely fits.
        const Obb obb = blockObb(b);
        std::array<double, 6> ext{1e30, -1e30, 1e30, -1e30, 1e30, -1e30};
        for (const double sz : {-1.0, 1.0}) {
            const double faceHX = sz < 0.0 ? 0.5 * b.size[0] : farHalfX;
            const double faceHY = sz < 0.0 ? 0.5 * b.size[1] : farHalfY;
            for (const double sx : {-1.0, 1.0})
                for (const double sy : {-1.0, 1.0}) {
                    const auto p = toWorld(obb, sx * faceHX, sy * faceHY, sz * obb.half[2]);
                    for (std::size_t c = 0; c < 3; ++c) {
                        ext[2 * c] = std::min(ext[2 * c], p[c]);
                        ext[2 * c + 1] = std::max(ext[2 * c + 1], p[c]);
                    }
                }
        }
        if (ext[0] < -cfg.envelope_half_x_mm - kEps || ext[1] > cfg.envelope_half_x_mm + kEps ||
            ext[2] < -cfg.envelope_half_y_mm - kEps || ext[3] > cfg.envelope_half_y_mm + kEps ||
            ext[4] < envStartMm - kEps || ext[5] > envEndMm + kEps)
            throw std::runtime_error(
                "MuonShieldConfig: " + where + " extends beyond the envelope: it spans x [" +
                std::to_string(ext[0]) + ", " + std::to_string(ext[1]) + "], y [" +
                std::to_string(ext[2]) + ", " + std::to_string(ext[3]) + "], z [" +
                std::to_string(ext[4]) + ", " + std::to_string(ext[5]) + "] mm");
    }

    // Distinct blocks must not overlap (touching is allowed). A full 3D test is
    // used rather than a Z-interval one: blocks may share a Z range when placed
    // at different x/y (e.g. arranged around an aperture). It is a
    // separating-axis test on oriented boxes, not on axis-aligned ones —
    // rotated blocks that intersect can have disjoint axis-aligned bounds.
    for (std::size_t i = 0; i < cfg.blocks.size(); ++i)
        for (std::size_t j = i + 1; j < cfg.blocks.size(); ++j)
            if (obbsOverlap(blockObb(cfg.blocks[i]), blockObb(cfg.blocks[j]), kEps))
                throw std::runtime_error("MuonShieldConfig: blocks " + std::to_string(i) + " and " +
                                         std::to_string(j) + " overlap in " + path);

    return cfg;
}

}  // namespace SHiPGeometry
