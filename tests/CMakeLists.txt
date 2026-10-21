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

# Integration test: build the geometry into SQLite and read it back.
add_test(
    NAME BuildAndReadBack
    COMMAND
        bash -c
        "$<TARGET_FILE:build_geometry> -o ship_test_readback.db &&
    $<TARGET_FILE:validate_geometry> ship_test_readback.db"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(BuildAndReadBack PROPERTIES FIXTURES_SETUP geometry_db)

# Overlap check: run gmclash (Geant4 CheckOverlaps) on the geometry that
# BuildAndReadBack wrote, and compare its report with known_clashes.json. With
# 100 random points per volume this takes a few minutes; skip it with
# `ctest -LE overlap`. After an intended change, refresh the list with
# check_clashes.py --update (see the script).
find_program(GMCLASH_EXECUTABLE gmclash)
find_package(Python3 COMPONENTS Interpreter)
if(GMCLASH_EXECUTABLE AND Python3_Interpreter_FOUND)
    add_test(
        NAME OverlapCheck
        COMMAND
            bash -c
            "rm -f gmclash_report.json &&
    ${GMCLASH_EXECUTABLE} -g ship_test_readback.db -n 100 -o gmclash_report.json > gmclash.log 2>&1 ||
    { tail -n 50 gmclash.log; exit 1; }
    ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_clashes.py gmclash_report.json ${CMAKE_CURRENT_SOURCE_DIR}/known_clashes.json"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(
        OverlapCheck
        PROPERTIES FIXTURES_REQUIRED geometry_db LABELS overlap TIMEOUT 1800
    )
else()
    message(WARNING "gmclash or Python 3 not found: no OverlapCheck test")
endif()

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
