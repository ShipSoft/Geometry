// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>

class GeoMaterial;
class GeoElement;

namespace SHiP::geometry {

/**
 * @brief Central material manager for the SHiP detector
 *
 * This class provides access to all materials and elements used in the SHiP detector geometry.
 * Materials are created once and shared across all subsystem factories.
 */
class Materials {
   public:
    Materials();
    ~Materials() = default;

    // Prevent copying (materials should be shared)
    Materials(const Materials&) = delete;
    Materials& operator=(const Materials&) = delete;

    /**
     * @brief Get a material by name
     * @param name Material name (e.g., "Air", "Concrete", "Tungsten")
     * @return Pointer to GeoMaterial or nullptr if not found
     */
    [[nodiscard]] GeoMaterial* getMaterial(std::string_view name) const;

    /**
     * @brief Get a material by name, throwing if not found
     * @param name Material name (e.g., "Air", "Concrete", "Tungsten")
     * @return Pointer to GeoMaterial (never nullptr)
     * @throws std::runtime_error if material not found
     */
    [[nodiscard]] GeoMaterial* requireMaterial(std::string_view name) const;

   private:
    void createElements();
    void createMaterials();

    std::map<std::string, GeoElement*> m_elements;
    std::map<std::string, GeoMaterial*, std::less<>> m_materials;
};

}  // namespace SHiP::geometry
