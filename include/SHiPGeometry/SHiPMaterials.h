// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Forwarding header for the pre-0.3.0 layout, removed in 0.4.0.
// SHiPGeometry/SHiPMaterials.h is now SHiP/geometry/Materials.h, and
// SHiPGeometry::SHiPMaterials is now SHiP::geometry::Materials.
#pragma once

#include "SHiP/geometry/Materials.h"

namespace SHiPGeometry = SHiP::geometry;

namespace SHiP::geometry {
using SHiPMaterials [[deprecated("use SHiP::geometry::Materials")]] = Materials;
}
