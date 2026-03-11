// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "UpstreamTagger/UpstreamTaggerFactory.h"

#include "SHiPGeometry/SHiPMaterials.h"
#include "UpstreamTagger/SHiPUBTManager.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>

namespace SHiPGeometry {

UpstreamTaggerFactory::UpstreamTaggerFactory(SHiPMaterials& materials) : m_materials(materials) {}

GeoVPhysVol* UpstreamTaggerFactory::build(SHiPUBTManager* manager) {
    const GeoMaterial* scint = m_materials.requireMaterial("Scintillator");

    auto* box = new GeoBox(s_halfX, s_halfY, s_halfZ);
    auto* log = new GeoLogVol("Upstream_Tagger", box, scint);
    auto* fpv = new GeoFullPhysVol(log);

    if (manager) {
        manager->setSlabVolume(fpv);
    }

    return fpv;
}

}  // namespace SHiPGeometry
