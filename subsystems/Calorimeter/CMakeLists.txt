# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# ── toml++ (header-only TOML parser) ──────────────────────────────────────────
# Pulled in via FetchContent so we don't vendor a 7000-line single header.
# Pinned to a release tag so subsequent fetches are reproducible.
include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# Stage calo.toml into the build directory so tests running from there find it.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/calo.toml
    ${CMAKE_CURRENT_BINARY_DIR}/calo.toml
    COPYONLY
)
# Also stage into the top-level build dir for test_builder and OverlapCheck.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/calo.toml
    ${CMAKE_BINARY_DIR}/calo.toml
    COPYONLY
)

add_library(
    Calorimeter
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

target_link_libraries(
    Calorimeter
    PUBLIC GeoModelCore::GeoModelKernel
    PRIVATE
        # toml++ is a build-time-only dependency: it's used by
        # CalorimeterConfig.cpp's parser internals and never appears in any
        # public header. Wrapping in $<BUILD_INTERFACE:...> keeps it out of
        # the install-time export set (otherwise CMake refuses to install
        # Calorimeter, since tomlplusplus isn't itself exportable).
        $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
)

# Bake the absolute source-tree path as a compile-time fallback so the factory
# can always find calo.toml even when CWD doesn't contain it.
target_compile_definitions(
    Calorimeter
    PRIVATE
        # Source-tree fallback (always valid during development and CI builds).
        CALO_TOML_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/calo.toml"
        # Install-time fallback: set to the installed data directory so that
        # deployed builds (where the source tree is absent) can find calo.toml.
        CALO_TOML_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/SHiPGeometry/calo.toml"
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/calo.toml
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
