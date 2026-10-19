// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

namespace SHiP::geometry::detail {

/**
 * @brief Reference every subsystem library, so the linker keeps them all.
 *
 * Does nothing at run time. Defined in the LinkSubsystems.cpp that
 * src/CMakeLists.txt generates from SHIP_SUBSYSTEMS; the registry entry points
 * call it so that a static link also pulls that object, and through it every
 * subsystem's registration, into the executable.
 */
void linkSubsystems();

}  // namespace SHiP::geometry::detail
