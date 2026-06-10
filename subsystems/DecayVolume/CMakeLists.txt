# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# toml++ — header-only TOML parser used by SBTConfig's parsing internals.
# Available on conda-forge (`tomlplusplus`) and in most distros.
find_package(tomlplusplus REQUIRED)

# Stage sbt.toml into the build directory so tests running from there find it.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sbt.toml
    ${CMAKE_CURRENT_BINARY_DIR}/sbt.toml
    COPYONLY
)
# Also stage into the top-level build dir for test_builder and OverlapCheck.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sbt.toml
    ${CMAKE_BINARY_DIR}/sbt.toml
    COPYONLY
)

add_library(
    DecayVolume
    src/DecayVolumeFactory.cpp
    src/SBTConfig.cpp
    src/SBTStructureBuilder.cpp
    src/SBTSensorBuilder.cpp
)

target_include_directories(
    DecayVolume
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    DecayVolume
    PUBLIC GeoModelCore::GeoModelKernel
    PRIVATE
        # toml++ is a build-time-only dependency: it's used by SBTConfig.cpp's
        # parser internals and never appears in any public header. Wrapping in
        # $<BUILD_INTERFACE:...> keeps it out of the install-time export set
        # (otherwise CMake refuses to install DecayVolume, since tomlplusplus
        # isn't itself exportable).
        $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
)

# Bake the absolute source-tree path as a compile-time fallback so the factory
# can always find sbt.toml even when CWD doesn't contain it.
target_compile_definitions(
    DecayVolume
    PRIVATE
        # Source-tree fallback (always valid during development and CI builds).
        SBT_TOML_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/sbt.toml"
        # Install-time fallback: set to the installed data directory so that
        # deployed builds (where the source tree is absent) can find sbt.toml.
        SBT_TOML_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/SHiPGeometry/sbt.toml"
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/sbt.toml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_decayvolume test_decayvolume.cpp)
    target_link_libraries(
        test_decayvolume
        PRIVATE DecayVolume SHiPGeometry Catch2::Catch2WithMain
    )
    catch_discover_tests(test_decayvolume
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
