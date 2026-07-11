# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Shared recipe: library (globs src/*.cpp, incl. SNDEnvelope.cpp), include dirs,
# kernel link, Catch test.
ship_add_subsystem(NeutrinoDetector)

# --- Extras: NeutrinoDetector parses SD.toml (the SND reservation envelope) via
# SNDEnvelope (toml++), the same way MuonShield/Calorimeter parse their configs.

find_package(tomlplusplus REQUIRED)

# toml++ is a build-time-only dependency (used only inside SNDEnvelope.cpp, never
# in a public header). BUILD_INTERFACE keeps it out of the install export set.
target_link_libraries(
    NeutrinoDetector
    PRIVATE $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
)

# Stage SD.toml next to the tests and into the top-level build dir (for
# test_builder / OverlapCheck), so it is found when running from either.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/SD.toml
    ${CMAKE_CURRENT_BINARY_DIR}/SD.toml
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/SD.toml
    ${CMAKE_BINARY_DIR}/SD.toml
    COPYONLY
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/SD.toml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry
)

# Absolute fallbacks for resolveConfigPath(), so the parser finds SD.toml when
# the CWD does not have it (mirrors the calorimeter's CALO_TOML_* defines).
target_compile_definitions(
    NeutrinoDetector
    PRIVATE
        SD_TOML_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/SD.toml"
        SD_TOML_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/SHiPGeometry/SD.toml"
)
