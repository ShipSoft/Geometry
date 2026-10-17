// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Forwarding header for the pre-0.3.0 layout, removed in 0.4.0.
// UpstreamTagger/SHiPUBTManager.h is now
// SHiP/geometry/UpstreamTagger/UpstreamTaggerManager.h, and
// SHiPGeometry::SHiPUBTManager is now SHiP::geometry::UpstreamTaggerManager.
#pragma once

#include "SHiP/geometry/UpstreamTagger/UpstreamTaggerManager.h"

namespace SHiPGeometry = SHiP::geometry;

namespace SHiP::geometry {
using SHiPUBTManager [[deprecated("use SHiP::geometry::UpstreamTaggerManager")]] =
    UpstreamTaggerManager;
}
