# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Subsystem self-registration: each subsystem library's REGISTER_SUBSYSTEM
# static initialiser is the only thing referencing it, so the toolchain's
# default --as-needed drops the library from the DT_NEEDED of every executable
# that links ship_geometry, and the registration never runs.
#
# For SHARED libraries the fix is --no-as-needed, and it has to reach this
# library's own link line as well as every consumer's. Its own line matters:
# nothing in DetectorBuilder.cpp names a subsystem symbol, so without the flag the
# subsystems drop out of libship_geometry's DT_NEEDED and any consumer that does
# not inherit the CMake interface (dlopen, a hand-written -lship_geometry,
# pkg-config, a plugin) loads a library with an empty registry.
#
# The flag is therefore a PUBLIC link *item*, placed directly before the
# subsystems, and not a link option. On a shared-library link CMake puts target
# LINK_OPTIONS before CMAKE_SHARED_LINKER_FLAGS, so an -Wl,--as-needed coming in
# through LDFLAGS (as conda-forge's compiler activation exports) would follow
# and override it. Link items come after the global flags.
#
# For STATIC libraries the flag does nothing — the objects need --whole-archive
# instead, which is not wired up here — so fail rather than silently produce an
# empty registry. This is orthogonal to platform, so it is checked first and
# independently.
if(NOT BUILD_SHARED_LIBS)
    message(
        FATAL_ERROR
        "BUILD_SHARED_LIBS=OFF is not supported: subsystem self-registration "
        "relies on -Wl,--no-as-needed for shared libraries and is not wired up "
        "for static archives (which would need --whole-archive). A static build "
        "would produce an empty subsystem registry at run time."
    )
endif()

# --no-as-needed is a GNU/binutils flag; Apple's ld neither needs nor accepts
# it (Mach-O has no --as-needed stripping to undo).
set(SHIP_NO_AS_NEEDED "")
if(UNIX AND NOT APPLE)
    set(SHIP_NO_AS_NEEDED "-Wl,--no-as-needed")
endif()

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
        mp-units::mp-units
        ${SHIP_NO_AS_NEEDED}
        ${SHIP_SUBSYSTEM_TARGETS}
)
