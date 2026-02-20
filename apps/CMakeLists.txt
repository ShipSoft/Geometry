# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_executable(build_geometry build_geometry.cpp)

target_link_libraries(build_geometry
    PRIVATE
        SHiPGeometry
        GeoModelIO::GeoModelIO
)