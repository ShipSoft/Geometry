# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# ship_add_toml_config(<target> <toml_file> <prefix>)
#
# Wires a subsystem's TOML config file into <target>, factoring out the setup
# shared by every config-driven subsystem (calorimeter, muon shield, neutrino
# detector):
#   * links toml++ as a build-time-only (BUILD_INTERFACE) private dependency,
#     so it stays out of the install export set;
#   * stages <toml_file> into both the subsystem build dir and the top-level
#     build dir, so tests find it whether run from either;
#   * defines <prefix>_TOML_DEFAULT_PATH (source tree) and
#     <prefix>_TOML_INSTALL_PATH (installed data dir) for resolveConfigPath();
#   * installs <toml_file> into the shared SHiPGeometry data dir.
#
# Call after ship_add_subsystem(<target> ...), which creates the target.
function(ship_add_toml_config target toml_file prefix)
    find_package(tomlplusplus REQUIRED)

    # toml++ is used only inside the parser .cpp, never in a public header;
    # BUILD_INTERFACE keeps it out of the install export set (tomlplusplus is
    # not itself exportable, so CMake would otherwise refuse to install target).
    target_link_libraries(
        ${target}
        PRIVATE $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${toml_file}
        ${CMAKE_CURRENT_BINARY_DIR}/${toml_file}
        COPYONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${toml_file}
        ${CMAKE_BINARY_DIR}/${toml_file}
        COPYONLY
    )

    # Absolute fallbacks so the parser finds the file when the CWD does not have
    # it. FULL_DATADIR is correct even when CMAKE_INSTALL_DATADIR is absolute.
    target_compile_definitions(
        ${target}
        PRIVATE
            ${prefix}_TOML_DEFAULT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${toml_file}"
            ${prefix}_TOML_INSTALL_PATH="${CMAKE_INSTALL_FULL_DATADIR}/SHiPGeometry/${toml_file}"
    )

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/${toml_file}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry
    )
endfunction()
