// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"
// Include subsystem factories when implemented

namespace SHiPGeometry {

SHiPGeometryBuilder::SHiPGeometryBuilder() = default;
SHiPGeometryBuilder::~SHiPGeometryBuilder() = default;

GeoPhysVol* SHiPGeometryBuilder::build() {
    // TODO: Build full geometry by calling subsystem factories
    // 1. Create world volume (Cavern)
    // 2. Place subsystems inside
    return nullptr;
}

} // namespace SHiPGeometry