# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(Cavern
    src/CavernFactory.cpp
)

target_include_directories(Cavern
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Cavern
    PUBLIC
        GeoModelCore::GeoModelKernel
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_cavern test_cavern.cpp)
    target_link_libraries(test_cavern PRIVATE Cavern SHiPGeometry Catch2::Catch2WithMain)
    catch_discover_tests(test_cavern)
endif()
