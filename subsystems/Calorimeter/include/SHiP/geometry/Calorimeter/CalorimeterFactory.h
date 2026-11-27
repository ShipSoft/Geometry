// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

#include <string_view>

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

namespace calo {
enum class LayerCode;
}

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

    /** Filled layers, each shared by every slot of its type in every module. */
    struct SharedLayers {
        GeoPhysVol* hplAlongY;  ///< HPL casing, fibres along Y
        GeoPhysVol* hplAlongX;  ///< HPL casing, fibres along X
        GeoPhysVol* widePvtH;   ///< wide PVT bars, long along X
        GeoPhysVol* widePvtV;   ///< wide PVT bars, long along Y
        GeoPhysVol* thinPsH;    ///< thin PS bars, long along X
        GeoPhysVol* thinPsV;    ///< thin PS bars, long along Y

        /** A shared layer with its name in volume names and its thickness. */
        struct Layer {
            std::string_view type;  ///< e.g. "thin_ps_h" or "hpl_y"
            GeoPhysVol* volume;
            double thickness;  ///< GeoModel units (mm)
        };

        /** The shared layer for a bar or HPL @p code; throws for anything else. */
        [[nodiscard]] Layer forCode(calo::LayerCode code) const;
    };

    class ModuleStack;

    /** Place one NX×NY tiled stack of layers inside @p container. */
    void buildStack(GeoPhysVol* container, const SharedLayers& shared, int moduleX, int moduleY,
                    double offsetX, double offsetY) const;
};

}  // namespace SHiP::geometry
