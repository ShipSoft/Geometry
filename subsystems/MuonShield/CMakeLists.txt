# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

ship_add_subsystem(MuonShield)

# The SND cavity is derived from NeutrinoDetectorFactory's constexpr sections,
# so the public header includes that factory's header. Only the header is
# used: there is no link dependency on the NeutrinoDetector library.
target_include_directories(
    ship_geometry_muonshield
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/subsystems/NeutrinoDetector/include>
)
