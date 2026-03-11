# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(Target
    src/TargetFactory.cpp
)

target_include_directories(Target
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Target
    PUBLIC
        GeoModelCore::GeoModelKernel
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_target test_target.cpp)
    target_link_libraries(test_target PRIVATE Target SHiPGeometry Catch2::Catch2WithMain)
    catch_discover_tests(test_target)
endif()
