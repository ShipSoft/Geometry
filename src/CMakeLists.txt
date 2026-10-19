# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Subsystem self-registration: nothing outside a subsystem library calls into
# it except its REGISTER_SUBSYSTEM static initialiser, so a linker running with
# --as-needed (conda-forge's default) or pulling objects from a static archive
# would drop the library and its registration would never run.
#
# REGISTER_SUBSYSTEM therefore also defines <Name>Factory_linkAnchor(), and
# LinkSubsystems.cpp, generated here from SHIP_SUBSYSTEMS, calls one per
# subsystem. That makes every subsystem a real dependency of ship_geometry, with
# no linker flags. A subsystem listed in SHIP_SUBSYSTEMS whose factory is not
# called <Name>Factory, or that does not call REGISTER_SUBSYSTEM, fails to link.
set(SHIP_ANCHOR_DECLARATIONS "")
set(SHIP_ANCHOR_CALLS "")
foreach(subsys ${SHIP_SUBSYSTEMS})
    string(
        APPEND SHIP_ANCHOR_DECLARATIONS
        "void ${subsys}Factory_linkAnchor();\n"
    )
    string(APPEND SHIP_ANCHOR_CALLS "    ${subsys}Factory_linkAnchor();\n")
endforeach()
configure_file(
    LinkSubsystems.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/LinkSubsystems.cpp
    @ONLY
)

# The detector-assembly library. Exported as SHiP::Geometry; the in-tree ALIAS
# carries the same spelling so source and installed consumers write one name.
add_library(
    ship_geometry
    Materials.cpp
    DetectorBuilder.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/LinkSubsystems.cpp
)
set_target_properties(ship_geometry PROPERTIES EXPORT_NAME Geometry)
add_library(SHiP::Geometry ALIAS ship_geometry)

target_include_directories(
    ship_geometry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    ship_geometry
    PUBLIC
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelRead
        GeoModelIO::GeoModelWrite
        mp-units::mp-units
        ${SHIP_SUBSYSTEM_TARGETS}
)
