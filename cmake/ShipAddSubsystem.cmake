# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# ship_add_subsystem(<Name> [TEST_WORKING_DIRECTORY <dir>] [SOURCES <files...>])
#
# Declares a subsystem library and its Catch2 test with the layout every
# subsystem shares: all src/*.cpp compiled into a library <Name>, the standard
# public include directories, a PUBLIC link to GeoModelCore::GeoModelKernel,
# and a test_<name> executable discovered via Catch.
#
# Subsystems that need more (extra link libraries, config staging, compile
# definitions, installs) call this first and then append to the <Name> target.
#
# Options:
#   SOURCES                 explicit source list (default: glob src/*.cpp).
#   TEST_WORKING_DIRECTORY  working directory for catch_discover_tests (needed
#                           by subsystems whose tests read staged data files).
function(ship_add_subsystem NAME)
    cmake_parse_arguments(ARG "" "TEST_WORKING_DIRECTORY" "SOURCES" ${ARGN})

    set(_sources ${ARG_SOURCES})
    if(NOT _sources)
        file(
            GLOB _sources
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        )
    endif()

    add_library(${NAME} ${_sources})

    target_include_directories(
        ${NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${NAME} PUBLIC GeoModelCore::GeoModelKernel)

    if(BUILD_TESTING)
        string(TOLOWER ${NAME} _lower)
        include(Catch)
        add_executable(test_${_lower} test_${_lower}.cpp)
        target_link_libraries(
            test_${_lower}
            PRIVATE ${NAME} SHiPGeometry Catch2::Catch2WithMain
        )
        if(ARG_TEST_WORKING_DIRECTORY)
            catch_discover_tests(
                test_${_lower}
                WORKING_DIRECTORY ${ARG_TEST_WORKING_DIRECTORY}
            )
        else()
            catch_discover_tests(test_${_lower})
        endif()
    endif()
endfunction()
