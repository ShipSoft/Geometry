// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <array>
#include <string>
#include <vector>

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;
struct MuonShieldConfig;

/**
 * @brief Factory for the MuonShield geometry.
 *
 * Builds an Air container spanning the muon-shield Z envelope and places an
 * explicit list of iron blocks inside it (from MS.toml). Each block is a box,
 * or a symmetric trapezoid/frustum (GeoTrd) when tapered, anchored by its
 * upstream face and optionally rotated.
 *
 * Other subsystems can be embedded as daughter volumes of the shield container
 * via embedDaughter() — used, for example, to nest the neutrino detector inside
 * the muon shield while keeping it an independent subsystem.
 *
 * The container is built centred on its own origin; SHiPGeometryBuilder places
 * it in the world at centreZ_mm() (the envelope centre from MS.toml).
 *
 * The config file is resolved at build() time:
 *   1. "MS.toml" relative to the current working directory (works when running
 *      from the build directory, where CMake stages the file).
 *   2. The absolute source-tree path baked in via MS_TOML_DEFAULT_PATH.
 *   3. The installed data-dir path via MS_TOML_INSTALL_PATH.
 */
class MuonShieldFactory {
   public:
    explicit MuonShieldFactory(SHiPMaterials& materials, std::string configPath = "MS.toml");
    ~MuonShieldFactory() = default;

    /**
     * @brief Embed an external volume as a daughter of the shield container.
     *
     * Registers @p daughter to be placed inside the shield at world-Z
     * @p worldCentreZ_mm. The daughter remains an independent subsystem; this
     * only nests it in the volume tree. Since the iron is defined by an explicit
     * block list, the caller is responsible for not listing blocks over the
     * daughter's footprint. Call before build().
     *
     * @param daughter        Pre-built volume (centred on its own origin).
     * @param worldCentreZ_mm


     centre where the daughter is placed (mm).
     * @param name            Name tag for the placement.
     */
    void embedDaughter(GeoPhysVol* daughter, double worldCentreZ_mm, const std::string& name);

    /**
     * @brief Reserve (carve) a box out of the shield iron via Boolean A - B.
     *
     * Registers a box to subtract from every magnet it intersects, so external
     * detectors (e.g. the neutrino detector, from SD.toml) can define their
     * space independently and have it removed from the iron at build() time.
     * Positions are in world/beamline coordinates. Call before build().
     *
     * @param worldCentre_mm  Box centre in world coordinates (mm).
     * @param size_mm         Full box dimensions {x, y, z} (mm).
     * @param rotation_deg    Optional extrinsic X->Y->Z rotation about the centre.
     */
    void reserveSpace(const std::array<double, 3>& worldCentre_mm,
                      const std::array<double, 3>& size_mm,
                      const std::array<double, 3>& rotation_deg = {0.0, 0.0, 0.0});

    /** Build and return the muon-shield container volume (centred on origin). */
    GeoPhysVol* build();

    /**
     * @brief World-Z centre (mm) at which the container should be placed.
     *
     * Valid only after build() has been called (it is read from MS.toml).
     */
    double centreZ_mm() const { return m_centreZ_mm; }

    /** Return the config path that will actually be opened (after resolution). */
    std::string resolvedConfigPath() const;

   private:
    struct EmbeddedDaughter {
        GeoPhysVol* volume;
        double worldCentreZ_mm;
        std::string name;
    };

    struct ReservedBox {
        std::array<double, 3> centre_mm;
        std::array<double, 3> size_mm;
        std::array<double, 3> rotation_deg;
    };

    SHiPMaterials& m_materials;
    std::string m_configPath;
    std::vector<EmbeddedDaughter> m_daughters;
    std::vector<ReservedBox> m_reservations;

    // World-Z centre of the container, populated by build() from the config.
    double m_centreZ_mm = 0.0;
};

}  // namespace SHiPGeometry
