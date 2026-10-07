# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# ship_add_subsystem(<Name> [SOURCES <files...>])
#
# Declares a subsystem library and its Catch2 test with the layout every
# subsystem shares: all src/*.cpp compiled into a library, the standard public
# include directories, a PUBLIC link to GeoModelCore::GeoModelKernel, and a
# test_<name> executable discovered via Catch.
#
# The library target is named ship_geometry_<name>, not <Name>. A target called
# Target or Magnet is a global name in the build tree of anything that builds
# this project from source, which is how the SHiP repositories consume each
# other during development. It is exported as SHiP::Geometry<Name>, and the same
# spelling is available in-tree as an ALIAS, so a consumer writes
# SHiP::GeometryCavern whether it found the installed package or added this
# project as a subdirectory.
#
# Subsystems that need more (extra link libraries, compile definitions,
# installs) call this first and then append to the ship_geometry_<name> target.
#
# Options:
#   SOURCES  explicit source list (default: glob src/*.cpp).
function(ship_add_subsystem NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    set(_sources ${ARG_SOURCES})
    if(NOT _sources)
        file(
            GLOB _sources
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        )
    endif()

    string(TOLOWER ${NAME} _lower)
    set(_target ship_geometry_${_lower})

    add_library(${_target} ${_sources})
    set_target_properties(${_target} PROPERTIES EXPORT_NAME Geometry${NAME})
    add_library(SHiP::Geometry${NAME} ALIAS ${_target})

    target_include_directories(
        ${_target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${_target} PUBLIC GeoModelCore::GeoModelKernel)

    if(BUILD_TESTING)
        include(Catch)
        add_executable(test_${_lower} test_${_lower}.cpp)
        target_link_libraries(
            test_${_lower}
            PRIVATE ${_target} ship_geometry Catch2::Catch2WithMain
        )
        catch_discover_tests(test_${_lower})
    endif()
endfunction()
