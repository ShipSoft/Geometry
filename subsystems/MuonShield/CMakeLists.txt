# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Shared recipe: library (globs src/*.cpp), include dirs, kernel link, Catch test.
ship_add_subsystem(MuonShield)

# --- Extras: MuonShield parses MS.toml via MuonShieldConfig (toml++), same as
# the calorimeter. Everything below mirrors subsystems/Calorimeter/CMakeLists.txt.

find_package(tomlplusplus REQUIRED)

# toml++ is a build-time-only dependency (used only inside MuonShieldConfig.cpp,
# never in a public header). BUILD_INTERFACE keeps it out of the install export.
target_link_libraries(
    MuonShield
    PRIVATE $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
)

# Stage MS.toml next to the tests and into the top-level build dir (for
# test_builder / OverlapCheck), so it is found when running from either.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/MS.toml
    ${CMAKE_CURRENT_BINARY_DIR}/MS.toml
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/MS.toml
    ${CMAKE_BINARY_DIR}/MS.toml
    COPYONLY
)

# Absolute fallbacks so the factory finds MS.toml when the CWD does not have it.
target_compile_definitions(
    MuonShield
    PRIVATE
        MS_TOML_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/MS.toml"
        MS_TOML_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/SHiPGeometry/MS.toml"
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/MS.toml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry
)
