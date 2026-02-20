// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"
#include <GeoModelKernel/GeoMaterial.h>
#include <map>
#include <string>

namespace SHiPGeometry {

class SHiPMaterials::Impl {
public:
    std::map<std::string, const GeoMaterial*> materials;
};

SHiPMaterials::SHiPMaterials() 
    : m_impl(std::make_unique<Impl>()) {
}

const GeoMaterial* SHiPMaterials::getMaterial(const std::string& name) const {
    auto it = m_impl->materials.find(name);
    return (it != m_impl->materials.end()) ? it->second : nullptr;
}

void SHiPMaterials::loadFromGDML(const std::string& gdmlFile) {
    // TODO: Implement GDML material loading
    // This will be populated in a later phase when we parse the GDML file
}

} // namespace SHiPGeometry