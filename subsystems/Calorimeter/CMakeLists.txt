# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Stage calo.cfg into the build directory so tests running from there find it.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/calo.cfg
    ${CMAKE_CURRENT_BINARY_DIR}/calo.cfg
    COPYONLY
)
# Also stage into the top-level build dir for test_builder and OverlapCheck.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/calo.cfg
    ${CMAKE_BINARY_DIR}/calo.cfg
    COPYONLY
)

add_library(Calorimeter
    src/CalorimeterFactory.cpp
    src/CalorimeterConfig.cpp
    src/CaloBarLayer.cpp
    src/CaloFibreHPLayer.cpp
)

target_include_directories(
    Calorimeter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Calorimeter PUBLIC GeoModelCore::GeoModelKernel)

# Bake the absolute source-tree path as a compile-time fallback so the factory
# can always find calo.cfg even when CWD doesn't contain it.
target_compile_definitions(Calorimeter
    PRIVATE
        # Source-tree fallback (always valid during development and CI builds).
        CALO_CFG_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/calo.cfg"
        # Install-time fallback: set to the installed data directory so that
        # deployed builds (where the source tree is absent) can find calo.cfg.
        CALO_CFG_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/SHiPGeometry/calo.cfg"
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/calo.cfg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_calorimeter test_calorimeter.cpp)
    target_link_libraries(
        test_calorimeter
        PRIVATE Calorimeter SHiPGeometry Catch2::Catch2WithMain
    )
    catch_discover_tests(test_calorimeter
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
