// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoVDetectorManager.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <vector>

namespace SHiPGeometry {

/**
 * @brief Detector manager for the Upstream Background Tagger (UBT).
 *
 * Stores the sensitive volumes of the UBT: drift tube gas volumes and
 * scintillator tile volumes, all registered as GeoFullPhysVol.
 *
 * Two sets of sensitive volumes:
 *  - Drift tube gas cylinders (ArCO2 70/30)
 *  - Polystyrene scintillator tiles (1×1×1 cm³)
 *
 * The legacy single-slab interface (setSlabVolume / getFullPV) is kept
 * for backwards compatibility with the monolithic placeholder.
 */
class SHiPUBTManager : public GeoVDetectorManager {
   public:
    SHiPUBTManager() = default;
    ~SHiPUBTManager() override = default;

    // ---- Legacy slab interface (monolithic placeholder) --------------------
    void setSlabVolume(GeoFullPhysVol* fpv) { m_slab = fpv; }
    GeoFullPhysVol* getFullPV() const { return m_slab; }

    // ---- Segmented detector interface --------------------------------------
    void addTubeGasVolume(GeoFullPhysVol* fpv) { m_tubeGasVolumes.push_back(fpv); }
    void addTileVolume(GeoFullPhysVol* fpv) { m_tileVolumes.push_back(fpv); }

    const std::vector<GeoFullPhysVol*>& getTubeGasVolumes() const { return m_tubeGasVolumes; }
    const std::vector<GeoFullPhysVol*>& getTileVolumes() const { return m_tileVolumes; }

    std::size_t numTubeGasVolumes() const { return m_tubeGasVolumes.size(); }
    std::size_t numTileVolumes() const { return m_tileVolumes.size(); }

    // ---- GeoVDetectorManager interface ------------------------------------
    unsigned int getNumTreeTops() const override {
        // If segmented volumes exist use them, otherwise fall back to slab
        const auto n = m_tubeGasVolumes.size() + m_tileVolumes.size();
        return n > 0 ? static_cast<unsigned int>(n) : (m_slab ? 1u : 0u);
    }

    PVConstLink getTreeTop(unsigned int i) const override {
        const auto nTubes = m_tubeGasVolumes.size();
        const auto nTiles = m_tileVolumes.size();
        if (nTubes + nTiles > 0) {
            if (i < nTubes)
                return PVConstLink(m_tubeGasVolumes[i]);
            if (i < nTubes + nTiles)
                return PVConstLink(m_tileVolumes[i - nTubes]);
        }
        return PVConstLink(m_slab);
    }

   private:
    GeoFullPhysVol* m_slab{nullptr};
    std::vector<GeoFullPhysVol*> m_tubeGasVolumes;
    std::vector<GeoFullPhysVol*> m_tileVolumes;
};

}  // namespace SHiPGeometry
