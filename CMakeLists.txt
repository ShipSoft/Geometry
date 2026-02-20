# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

cmake_minimum_required(VERSION 3.16)

project(SHiPGeometry
    VERSION 1.0.0
    DESCRIPTION "SHiP detector geometry implementation using GeoModel"
    LANGUAGES CXX
)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required GeoModel packages
find_package(GeoModelCore REQUIRED)
find_package(GeoModelKernel REQUIRED)
find_package(GeoModelIO REQUIRED)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(subsystems)
add_subdirectory(apps)
add_subdirectory(tests)