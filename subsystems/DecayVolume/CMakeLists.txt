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

# Library + test (all src/*.cpp); the test runs from the build dir so it finds
# the staged sbt.toml.
ship_add_subsystem(DecayVolume TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    DecayVolume
    # toml++ is a build-time-only dependency: it's used by SBTConfig.cpp's
    # parser internals and never appears in any public header. Wrapping in
    # $<BUILD_INTERFACE:...> keeps it out of the install-time export set
    # (otherwise CMake refuses to install DecayVolume, since tomlplusplus
    # isn't itself exportable).
    PRIVATE $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
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
