# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

cmake_minimum_required(VERSION 3.16)

project(SHiPGeometry
    VERSION 1.0.0
    DESCRIPTION "SHiP detector geometry implementation using GeoModel"
    LANGUAGES CXX
)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find GeoModel packages
find_package(GeoModelCore REQUIRED)
find_package(GeoModelIO REQUIRED)
find_package(GeoModelTools REQUIRED)

# Testing
include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(subsystems)
add_subdirectory(apps)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Export all targets (main library + subsystem libraries)
install(TARGETS
        SHiPGeometry
        Cavern Target MuonShield Magnet DecayVolume
        Trackers Calorimeter UpstreamTagger TimingDetector
    EXPORT SHiPGeometryTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install main headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install subsystem headers (needed for transitive include dirs in exported targets)
foreach(subsys Cavern Target MuonShield Magnet DecayVolume Trackers Calorimeter UpstreamTagger TimingDetector)
    install(DIRECTORY subsystems/${subsys}/include/${subsys}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endforeach()

# Install the export targets file
install(EXPORT SHiPGeometryTargets
    FILE SHiPGeometryTargets.cmake
    NAMESPACE SHiPGeometry::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometry
)

# Generate package config
configure_package_config_file(
    cmake/SHiPGeometryConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometry
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPGeometryConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPGeometry
)
