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

# Subsystem self-registration: each subsystem library's REGISTER_SUBSYSTEM
# static initialiser is the only thing referencing it, so the toolchain's
# default --as-needed drops the library from the DT_NEEDED of every executable
# that links SHiPGeometry, and the registration never runs.
#
# For SHARED libraries the fix is --no-as-needed on the *consumer's* link line
# (INTERFACE), where the entries were being dropped. For STATIC libraries that
# flag does nothing — the objects need --whole-archive instead, which is not
# wired up here — so warn rather than silently produce an empty registry. This
# is orthogonal to platform, so it is checked first and independently.
if(NOT BUILD_SHARED_LIBS)
    message(
        WARNING
        "Static build detected (BUILD_SHARED_LIBS=OFF): subsystem "
        "self-registration relies on -Wl,--no-as-needed for shared libraries "
        "and is not wired up for static archives (which would need "
        "--whole-archive). build_geometry and the tests may find an empty "
        "subsystem registry at run time. Prefer BUILD_SHARED_LIBS=ON."
    )
endif()

# --no-as-needed is a GNU/binutils flag; Apple's ld neither needs nor accepts
# it (Mach-O has no --as-needed stripping to undo).
if(UNIX AND NOT APPLE)
    target_link_options(SHiPGeometry INTERFACE "-Wl,--no-as-needed")
endif()
