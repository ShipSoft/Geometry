// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

/**
 * @brief Factory for the UpstreamTagger (upstream veto tagger) geometry
 */
class UpstreamTaggerFactory {
public:
    UpstreamTaggerFactory() = default;
    ~UpstreamTaggerFactory() = default;

    /**
     * @brief Build the UpstreamTagger geometry
     * @return Pointer to the physical volume
     */
    GeoPhysVol* build();
};

} // namespace SHiPGeometry