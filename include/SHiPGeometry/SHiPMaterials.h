// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <map>
#include <string>

class GeoMaterial;
class GeoElement;

namespace SHiPGeometry {

/**
 * @brief Central material manager for the SHiP detector
 *
 * This class provides access to all materials and elements used in the SHiP detector geometry.
 * Materials are created once and shared across all subsystem factories.
 */
class SHiPMaterials {
   public:
    SHiPMaterials();
    ~SHiPMaterials() = default;

    // Prevent copying (materials should be shared)
    SHiPMaterials(const SHiPMaterials&) = delete;
    SHiPMaterials& operator=(const SHiPMaterials&) = delete;

    /**
     * @brief Get an element by name
     * @param name Element name (e.g., "Nitrogen", "Oxygen")
     * @return Pointer to GeoElement or nullptr if not found
     */
    GeoElement* getElement(const std::string& name) const;

    /**
     * @brief Get a material by name
     * @param name Material name (e.g., "Air", "Concrete", "Tungsten")
     * @return Pointer to GeoMaterial or nullptr if not found
     */
    GeoMaterial* getMaterial(const std::string& name) const;

    /**
     * @brief Get a material by name, throwing if not found
     * @param name Material name (e.g., "Air", "Concrete", "Tungsten")
     * @return Pointer to GeoMaterial (never nullptr)
     * @throws std::runtime_error if material not found
     */
    GeoMaterial* requireMaterial(const std::string& name) const;

   private:
    void createElements();
    void createMaterials();

    std::map<std::string, GeoElement*> m_elements;
    std::map<std::string, GeoMaterial*> m_materials;
};

}  // namespace SHiPGeometry
