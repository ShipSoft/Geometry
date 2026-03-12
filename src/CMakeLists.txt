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
        Cavern
        Target
        MuonShield
        DecayVolume
        UpstreamTagger
        Trackers
        Magnet
        TimingDetector
        Calorimeter
)
