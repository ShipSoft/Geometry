# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(Magnet
    src/MagnetFactory.cpp
)

target_include_directories(Magnet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Magnet
    PUBLIC
        GeoModelCore::GeoModelKernel
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_magnet test_magnet.cpp)
    target_link_libraries(test_magnet PRIVATE Magnet SHiPGeometry Catch2::Catch2WithMain)
    catch_discover_tests(test_magnet)
endif()
