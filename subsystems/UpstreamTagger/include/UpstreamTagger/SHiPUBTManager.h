// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoVDetectorManager.h>
#include <GeoModelKernel/GeoVPhysVol.h>

namespace SHiPGeometry {

/**
 * @brief Detector manager for the Upstream Background Tagger (UBT).
 *
 * Stores the GeoFullPhysVol tile-plane container as the single sensitive
 * tree-top and satisfies the GeoVDetectorManager interface for downstream
 * Geant4 integration.
 */
class SHiPUBTManager : public GeoVDetectorManager {
   public:
    SHiPUBTManager() = default;
    ~SHiPUBTManager() override = default;

    void setContainerVolume(GeoFullPhysVol* fpv) { m_container = fpv; }
    GeoFullPhysVol* getFullPV() const { return m_container; }

    unsigned int getNumTreeTops() const override { return m_container ? 1u : 0u; }

    PVConstLink getTreeTop(unsigned int /*i*/) const override { return PVConstLink(m_container); }

   private:
    GeoFullPhysVol* m_container{nullptr};
};

}  // namespace SHiPGeometry
