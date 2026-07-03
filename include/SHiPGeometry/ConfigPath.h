// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <filesystem>
#include <string>

namespace SHiPGeometry {

/// Resolve a config-file path against the repo's standard fallback chain.
///
/// Order: the path as-is if it is absolute or exists relative to the current
/// working directory; then the build-time source-tree path; then the installed
/// data path. If none exist, @p path is returned unchanged so the caller's
/// reader emits the not-found error.
inline std::string resolveConfigPath(const std::string& path, const std::string& srcFallback,
                                     const std::string& installFallback) {
    namespace fs = std::filesystem;
    if (!path.empty() && path.front() == '/')
        return path;  // already absolute
    if (fs::exists(path))
        return path;  // relative to CWD
    if (!srcFallback.empty() && fs::exists(srcFallback))
        return srcFallback;
    if (!installFallback.empty() && fs::exists(installFallback))
        return installFallback;
    return path;
}

}  // namespace SHiPGeometry
