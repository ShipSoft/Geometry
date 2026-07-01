# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

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
        NeutrinoDetector
        DecayVolume
        UpstreamTagger
        Trackers
        Magnet
        TimingDetector
        Calorimeter
)

# The subsystems are referenced only through their REGISTER_SUBSYSTEM static
# initialisers, so the toolchain's default --as-needed drops them from the
# DT_NEEDED of every executable that links SHiPGeometry, and their
# registrations never run. INTERFACE puts --no-as-needed on the *consumer's*
# link line, which is where the entries were being dropped.
target_link_options(SHiPGeometry INTERFACE "-Wl,--no-as-needed")
