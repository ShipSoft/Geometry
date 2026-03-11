# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(UpstreamTagger src/UpstreamTaggerFactory.cpp)

target_include_directories(
    UpstreamTagger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(UpstreamTagger PUBLIC GeoModelCore::GeoModelKernel)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_upstreamtagger test_upstreamtagger.cpp)
    target_link_libraries(
        test_upstreamtagger
        PRIVATE UpstreamTagger SHiPGeometry Catch2::Catch2WithMain
    )
    catch_discover_tests(test_upstreamtagger)
endif()
