# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Subsystem directories, driven by SHIP_SUBSYSTEMS (defined in the top-level
# CMakeLists, in implementation order).
foreach(subsys ${SHIP_SUBSYSTEMS})
    add_subdirectory(${subsys})
endforeach()
