// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the Calorimeter (ECAL + HCAL) geometry.
 *
 * Creates a fixed-size container volume matching the SHiP envelope
 * (3.00 × 3.50 × 1.45 m half-sizes, centred at Z = 98 320 mm) and fills
 * it with the layer-by-layer geometry defined in CalorimeterConstants.h.
 */
class CalorimeterFactory {
   public:
    explicit CalorimeterFactory(Materials& materials);
    ~CalorimeterFactory() = default;

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * The container is fixed to the SHiP subsystem envelope, centred at
     * z = 98.32 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        using units::m;
        return {.name = "Calorimeter",
                .node = "/SHiP/calorimeter",
                .id = 8,
                .x_mm = 0.0,
                .y_mm = 0.0,
                .z_mm = gm(98.32 * m),
                .isWorld = false};
    }
    /** Build and return the calorimeter container volume. */
    [[nodiscard]] GeoPhysVol* build();

   private:
    Materials& m_materials;

    /** The two HPL casings, each shared by every layer of its orientation. */
    struct HplCasings {
        GeoPhysVol* alongY;  ///< fibres along Y
        GeoPhysVol* alongX;  ///< fibres along X
    };

    /** Place one NX×NY tiled stack of layers inside @p container. */
    void buildStack(GeoPhysVol* container, const HplCasings& hpl, int moduleX, int moduleY,
                    double offsetX, double offsetY) const;
};

}  // namespace SHiP::geometry
