# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

include(Catch)

add_executable(test_builder test_builder.cpp)
target_link_libraries(test_builder PRIVATE ship_geometry Catch2::Catch2WithMain)
catch_discover_tests(test_builder)

add_executable(test_materials test_materials.cpp)
target_link_libraries(
    test_materials
    PRIVATE ship_geometry Catch2::Catch2WithMain
)
catch_discover_tests(test_materials)

add_executable(test_consistency test_consistency.cpp)
target_link_libraries(
    test_consistency
    PRIVATE ship_geometry Catch2::Catch2WithMain
)
catch_discover_tests(test_consistency)

add_executable(test_registry test_registry.cpp)
target_link_libraries(
    test_registry
    PRIVATE ship_geometry Catch2::Catch2WithMain
)
# Hand the test the build's subsystem list, so that a subsystem which is linked
# but never calls REGISTER_SUBSYSTEM fails the test instead of silently
# vanishing from the geometry.
list(JOIN SHIP_SUBSYSTEMS "," _ship_subsystems_csv)
target_compile_definitions(
    test_registry
    PRIVATE "SHIP_EXPECTED_SUBSYSTEMS=\"${_ship_subsystems_csv}\""
)
catch_discover_tests(test_registry)

# Integration test: build the geometry into SQLite and read it back. This does
# not check for overlaps; see #121.
add_test(
    NAME BuildAndReadBack
    COMMAND
        bash -c
        "$<TARGET_FILE:build_geometry> -o ship_test_readback.db &&
    $<TARGET_FILE:validate_geometry> ship_test_readback.db"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# libship_geometry's own DT_NEEDED must list every subsystem, or a consumer that
# does not link it through CMake (dlopen, plain -lship_geometry) gets an empty
# registry. See the link-anchor note in src/CMakeLists.txt. Only meaningful for
# shared libraries, and the RTLD_NOLOAD lookup by soname is ELF-specific.
if(BUILD_SHARED_LIBS AND UNIX AND NOT APPLE)
    add_executable(test_dt_needed test_dt_needed.cpp)
    target_link_libraries(test_dt_needed PRIVATE ${CMAKE_DL_LIBS})
    add_dependencies(test_dt_needed ship_geometry)
    set(_subsystem_sonames "")
    foreach(subsys ${SHIP_SUBSYSTEM_TARGETS})
        list(APPEND _subsystem_sonames "$<TARGET_SONAME_FILE_NAME:${subsys}>")
    endforeach()
    add_test(
        NAME SubsystemsInDtNeeded
        COMMAND
            test_dt_needed $<TARGET_FILE:ship_geometry> ${_subsystem_sonames}
    )
endif()
