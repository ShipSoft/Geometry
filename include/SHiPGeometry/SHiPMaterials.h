// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <memory>
#include <string>

class GeoMaterial;

namespace SHiPGeometry {

/**
 * @brief Material definitions for the SHiP detector
 * 
 * This class provides access to all materials used in the SHiP detector geometry.
 * Materials are loaded from the GDML file and made available through this interface.
 */
class SHiPMaterials {
public:
    SHiPMaterials();
    ~SHiPMaterials() = default;

    /**
     * @brief Get a material by name
     * @param name Material name as defined in GDML
     * @return Pointer to GeoMaterial or nullptr if not found
     */
    const GeoMaterial* getMaterial(const std::string& name) const;

    /**
     * @brief Load materials from GDML file
     * @param gdmlFile Path to GDML file containing material definitions
     */
    void loadFromGDML(const std::string& gdmlFile);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace SHiPGeometry