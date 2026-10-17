# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# The detector-assembly library. Exported as SHiP::Geometry; the in-tree ALIAS
# carries the same spelling so source and installed consumers write one name.
add_library(ship_geometry Materials.cpp DetectorBuilder.cpp)
set_target_properties(ship_geometry PROPERTIES EXPORT_NAME Geometry)
add_library(SHiP::Geometry ALIAS ship_geometry)

target_include_directories(
    ship_geometry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    ship_geometry
    PUBLIC
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelRead
        GeoModelIO::GeoModelWrite
        ${SHIP_SUBSYSTEM_TARGETS}
)
