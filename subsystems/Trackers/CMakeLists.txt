# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(Trackers src/TrackersFactory.cpp)

target_include_directories(
    Trackers
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Trackers PUBLIC GeoModelCore::GeoModelKernel)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_trackers test_trackers.cpp)
    target_link_libraries(
        test_trackers
        PRIVATE Trackers SHiPGeometry Catch2::Catch2WithMain
    )
    catch_discover_tests(test_trackers)
endif()
