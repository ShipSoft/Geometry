# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_executable(build_geometry build_geometry.cpp)
target_link_libraries(
    build_geometry
    PRIVATE
        SHiPGeometry
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelDBManager
        GeoModelIO::GeoModelWrite
)

add_executable(validate_geometry validate_geometry.cpp)
target_link_libraries(validate_geometry PRIVATE SQLite3::SQLite3)

include(GNUInstallDirs)
install(
    TARGETS build_geometry validate_geometry
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
