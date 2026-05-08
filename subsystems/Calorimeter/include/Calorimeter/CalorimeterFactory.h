// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <string>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;
struct CalorimeterConfig;

/**
 * @brief Factory for the Calorimeter (ECAL + HCAL) geometry.
 *
 * Creates a fixed-size container volume matching the SHiP envelope
 * (3.00 × 3.50 × 1.45 m half-sizes, centred at Z = 98 320 mm) and fills
 * it with the real layer-by-layer geometry driven by calo.toml.
 *
 * The config file is resolved at build() time:
 *   1. "calo.toml" relative to the current working directory (works when
 *      running from the build directory, where CMake stages the file).
 *   2. The absolute source-tree path baked in at compile time via
 *      CALO_TOML_DEFAULT_PATH (always valid during development).
 */
class CalorimeterFactory {
   public:
    explicit CalorimeterFactory(SHiPMaterials& materials, std::string configPath = "calo.toml");
    ~CalorimeterFactory() = default;

    /** Build and return the calorimeter container volume. */
    GeoPhysVol* build();

    /**
     * @brief Compute the total Z extent of one ECAL+gap+HCAL stack (mm).
     *
     * Public so tests and placement code can query the stack thickness
     * independently of a full build().
     */
    static double totalStackZ(const CalorimeterConfig& cfg);

    /** Return the config path that will actually be opened (after resolution). */
    std::string resolvedConfigPath() const;

   private:
    SHiPMaterials& m_materials;
    std::string m_configPath;

    /** Place one NX×NY tiled stack of layers inside @p container. */
    void buildStack(GeoPhysVol* container, const CalorimeterConfig& cfg, int mx, int my,
                    double offsetX, double offsetY) const;

    // ── Fixed container dimensions (mm) ─────────────────────────────────
    // These match the SHiP subsystem envelope from subsystem_envelopes.csv
    // and must not change — tests and the consistency check depend on them.
    static constexpr double s_containerHalfX = 3000.0;  // 3.00 m
    static constexpr double s_containerHalfY = 3500.0;  // 3.50 m
    static constexpr double s_containerHalfZ = 1450.0;  // 1.45 m
};

}  // namespace SHiPGeometry
