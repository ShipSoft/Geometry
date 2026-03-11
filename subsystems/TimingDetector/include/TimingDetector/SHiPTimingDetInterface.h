// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoVFullPhysVol.h>
#include <GeoModelXml/GmxInterface.h>

#include <map>
#include <string>
#include <vector>

namespace SHiPGeometry {

/**
 * @brief GmxInterface implementation for the SHiP Timing Detector.
 *
 * Collects each sensitive bar registered by Gmx2Geo via addSensor().
 * After Gmx2Geo construction, barCount() returns the total number of bars.
 */
class SHiPTimingDetInterface : public GmxInterface {
   public:
    void addSensor(const std::string& /*name*/, std::map<std::string, int>& /*index*/, int /*id*/,
                   GeoVFullPhysVol* fpv) override {
        m_bars.push_back(fpv);
    }

    int barCount() const { return static_cast<int>(m_bars.size()); }

    const std::vector<GeoVFullPhysVol*>& bars() const { return m_bars; }

   private:
    std::vector<GeoVFullPhysVol*> m_bars;
};

}  // namespace SHiPGeometry
