# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

include(Catch)

add_executable(test_builder test_builder.cpp)
target_link_libraries(test_builder PRIVATE SHiPGeometry Catch2::Catch2WithMain)
catch_discover_tests(test_builder)

add_executable(test_materials test_materials.cpp)
target_link_libraries(
    test_materials
    PRIVATE SHiPGeometry Catch2::Catch2WithMain
)
catch_discover_tests(test_materials)

add_executable(test_consistency test_consistency.cpp)
target_link_libraries(
    test_consistency
    PRIVATE SHiPGeometry Catch2::Catch2WithMain
)
catch_discover_tests(test_consistency)

add_executable(test_registry test_registry.cpp)
target_link_libraries(test_registry PRIVATE SHiPGeometry Catch2::Catch2WithMain)
catch_discover_tests(test_registry)

add_executable(test_configpath test_configpath.cpp)
target_link_libraries(
    test_configpath
    PRIVATE SHiPGeometry Catch2::Catch2WithMain
)
catch_discover_tests(test_configpath)

# Integration test: build geometry into SQLite then run overlap check
add_test(
    NAME OverlapCheck
    COMMAND
        bash -c
        "$<TARGET_FILE:build_geometry> -o ship_test_overlap.db &&
    $<TARGET_FILE:validate_geometry> ship_test_overlap.db"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# libSHiPGeometry's own DT_NEEDED must list every subsystem, or a consumer that
# does not link it through CMake (dlopen, plain -lSHiPGeometry) gets an empty
# registry. See the --no-as-needed note in src/CMakeLists.txt; this is the
# same platform gate.
if(UNIX AND NOT APPLE)
    add_executable(test_dt_needed test_dt_needed.cpp)
    target_link_libraries(test_dt_needed PRIVATE ${CMAKE_DL_LIBS})
    add_dependencies(test_dt_needed SHiPGeometry)
    set(_subsystem_sonames "")
    foreach(subsys ${SHIP_SUBSYSTEMS})
        list(APPEND _subsystem_sonames "$<TARGET_SONAME_FILE_NAME:${subsys}>")
    endforeach()
    add_test(
        NAME SubsystemsInDtNeeded
        COMMAND test_dt_needed $<TARGET_FILE:SHiPGeometry> ${_subsystem_sonames}
    )
endif()
