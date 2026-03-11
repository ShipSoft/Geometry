// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "TimingDetector/TimingDetectorFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "TimingDetector/SHiPTimingDetInterface.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelXml/Gmx2Geo.h>

namespace SHiPGeometry {

TimingDetectorFactory::TimingDetectorFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoPhysVol* TimingDetectorFactory::build() {
    auto* air = m_materials.requireMaterial("Air");

    auto* containerBox = new GeoBox(s_containerHalfX, s_containerHalfY, s_containerHalfZ);
    auto* containerLog = new GeoLogVol("Timing_Detector", containerBox, air);
    auto* containerPhys = new GeoPhysVol(containerLog);

    SHiPTimingDetInterface iface;
    Gmx2Geo gmx(TIMING_DETECTOR_GMX, containerPhys, iface);

    m_barCount = iface.barCount();

    return containerPhys;
}

}  // namespace SHiPGeometry
