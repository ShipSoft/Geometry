// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/ConfigPath.h"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>

using SHiPGeometry::resolveConfigPath;

namespace fs = std::filesystem;

namespace {

/// RAII sandbox: a unique temp directory removed on destruction, with helpers to
/// materialise regular files and subdirectories inside it.
struct Sandbox {
    fs::path root;

    Sandbox() : root(fs::temp_directory_path() / "ship_configpath_test") {
        fs::remove_all(root);
        fs::create_directories(root);
    }
    ~Sandbox() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    std::string file(const std::string& name) const {
        fs::path p = root / name;
        std::ofstream(p) << "x";
        return p.string();
    }
    std::string dir(const std::string& name) const {
        fs::path p = root / name;
        fs::create_directories(p);
        return p.string();
    }
    std::string missing(const std::string& name) const { return (root / name).string(); }
};

// A relative path that does not exist in the current working directory. Used as
// the primary `path` argument in fallback tests: it is neither absolute (so it
// does not hit the absolute passthrough) nor present (so the chain is consulted).
constexpr const char* kAbsentRel = "ship_configpath_absent_probe.toml";

}  // namespace

TEST_CASE("ConfigPathTest.AbsolutePassthrough", "[configpath]") {
    // An absolute path is returned verbatim even when it does not exist.
    const std::string abs = "/nonexistent/ship/config.toml";
    CHECK(resolveConfigPath(abs, "", "") == abs);
}

TEST_CASE("ConfigPathTest.RelativeInCwd", "[configpath]") {
    Sandbox sb;
    sb.file("cwd.toml");
    const std::string src = sb.file("src.toml");

    const fs::path saved = fs::current_path();
    fs::current_path(sb.root);
    // A CWD-relative regular file wins over the fallbacks: proves the relative
    // branch fired rather than the not-found passthrough returning the input.
    CHECK(resolveConfigPath("cwd.toml", src, "") == "cwd.toml");  // NOLINT(readability/check)
    fs::current_path(saved);
}

TEST_CASE("ConfigPathTest.SrcFallbackHit", "[configpath]") {
    Sandbox sb;
    const std::string src = sb.file("src.toml");
    const std::string install = sb.missing("install.toml");

    CHECK(resolveConfigPath(kAbsentRel, src, install) == src);
}

TEST_CASE("ConfigPathTest.InstallFallbackHit", "[configpath]") {
    Sandbox sb;
    const std::string src = sb.missing("src.toml");
    const std::string install = sb.file("install.toml");

    CHECK(resolveConfigPath(kAbsentRel, src, install) == install);
}

TEST_CASE("ConfigPathTest.NotFoundPassthrough", "[configpath]") {
    Sandbox sb;

    CHECK(resolveConfigPath(kAbsentRel, sb.missing("src.toml"), sb.missing("install.toml")) ==
          kAbsentRel);
}

TEST_CASE("ConfigPathTest.DirectoryIsRejected", "[configpath]") {
    Sandbox sb;
    // A directory sitting at a fallback path must be skipped, not returned as a
    // config file: the chain falls through to the regular-file install fallback.
    const std::string srcDir = sb.dir("src.toml");
    const std::string install = sb.file("install.toml");

    CHECK(resolveConfigPath(kAbsentRel, srcDir, install) == install);
}
