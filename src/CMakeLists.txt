# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Create SHiPGeometry library
add_library(SHiPGeometry SHiPMaterials.cpp SHiPGeometry.cpp)

target_include_directories(
    SHiPGeometry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    SHiPGeometry
    PUBLIC
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelRead
        GeoModelIO::GeoModelWrite
        # Force-link the subsystem archives: each registers itself at
        # static-init via REGISTER_SUBSYSTEM, and nothing here references
        # their symbols, so without --whole-archive the linker would drop
        # them and the registry would be empty. (GNU/Clang syntax; for
        # CMake >= 3.24 prefer $<LINK_LIBRARY:WHOLE_ARCHIVE,...>.)
        "-Wl,--whole-archive"
        Cavern
        Target
        MuonShield
        NeutrinoDetector
        DecayVolume
        UpstreamTagger
        Trackers
        Magnet
        TimingDetector
        Calorimeter
        "-Wl,--no-whole-archive"
)
