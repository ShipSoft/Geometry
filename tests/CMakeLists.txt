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

# Integration test: build geometry into SQLite then run overlap check
add_test(
    NAME OverlapCheck
    COMMAND
        bash -c
        "$<TARGET_FILE:build_geometry> ship_test_overlap.db &&
    $<TARGET_FILE:validate_geometry> ship_test_overlap.db"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
