// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Calorimeter (ECAL + HCAL) geometry.
 *
 * Creates a fixed-size container volume matching the SHiP envelope
 * (3.00 × 3.50 × 1.45 m half-sizes, centred at Z = 98 320 mm) and fills
 * it with the layer-by-layer geometry defined in CalorimeterConstants.h.
 */
class CalorimeterFactory {
   public:
    explicit CalorimeterFactory(SHiPMaterials& materials);
    ~CalorimeterFactory() = default;

    /** Build and return the calorimeter container volume. */
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    /** Place one NX×NY tiled stack of layers inside @p container. */
    void buildStack(GeoPhysVol* container, int moduleX, int moduleY, double offsetX,
                    double offsetY) const;
};

}  // namespace SHiPGeometry
