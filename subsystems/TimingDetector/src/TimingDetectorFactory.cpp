// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "TimingDetector/TimingDetectorFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "TimingDetector/SHiPTimingDetInterface.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelXml/Gmx2Geo.h>

#include <filesystem>
#include <string>

// Build-tree fallback path baked in by CMake so out-of-source builds always
// find timing_detector.gmx even when the CWD doesn't contain a copy of it.
#ifndef TIMING_DETECTOR_GMX_DEFAULT_PATH
#define TIMING_DETECTOR_GMX_DEFAULT_PATH "timing_detector.gmx"
#endif
// Install-time data directory path, set by CMake during install configuration.
#ifndef TIMING_DETECTOR_GMX_INSTALL_PATH
#define TIMING_DETECTOR_GMX_INSTALL_PATH ""
#endif

namespace SHiPGeometry {

// ── file-scope helper ────────────────────────────────────────────────────────

static std::string resolveGmxPath() {
    const std::string srcFallback = TIMING_DETECTOR_GMX_DEFAULT_PATH;
    if (std::filesystem::exists(srcFallback))
        return srcFallback;  // build-tree fallback
    const std::string installFallback = TIMING_DETECTOR_GMX_INSTALL_PATH;
    if (!installFallback.empty() && std::filesystem::exists(installFallback))
        return installFallback;  // installed data dir
    return srcFallback;          // give up — Gmx2Geo will emit the error
}

TimingDetectorFactory::TimingDetectorFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TimingDetectorFactory::build() {
    auto* air = m_materials.requireMaterial("Air");

    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("/SHiP/timing_detector", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    SHiPTimingDetInterface iface;
    Gmx2Geo gmx(resolveGmxPath(), containerPhys, iface);

    m_barCount = iface.barCount();

    return containerPhys;
}

}  // namespace SHiPGeometry
