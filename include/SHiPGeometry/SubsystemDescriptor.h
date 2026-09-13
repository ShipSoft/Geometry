// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

namespace SHiPGeometry {

/**
 * @brief A subsystem's self-description: everything the assembler needs.
 *
 * This is the "required input to the geometry builder" that each subsystem
 * yields about itself. It is a plain data type with no GeoModel or standard-
 * library dependency, so it is cheap to include from the factory headers
 * (which only need it as the return type of `descriptor()`). The registry
 * machinery that consumes it lives in SubsystemRegistry.h, included by the
 * factory implementations.
 *
 * Translations are in millimetres from the world origin.
 */
struct SubsystemDescriptor {
    const char* name;      ///< registry key / CLI name, e.g. "Calorimeter"
    const char* node;      ///< GeoNameTag, e.g. "/SHiP/calorimeter"
    int id;                ///< GeoIdentifierTag
    double x_mm;           ///< placement translation X (mm)
    double y_mm;           ///< placement translation Y (mm)
    double z_mm;           ///< placement translation Z, beam direction (mm)
    bool isWorld = false;  ///< true for the mother/world volume (the cavern)
};

}  // namespace SHiPGeometry
