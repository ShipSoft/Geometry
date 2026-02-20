# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(Target
    src/TargetFactory.cpp
)

target_include_directories(Target
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Target
    PUBLIC
        GeoModelCore::GeoModelCore
        GeoModelKernel::GeoModelKernel
)