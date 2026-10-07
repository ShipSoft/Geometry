// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the DecayVolume subsystem (decay region + SBT).
 *
 * Builds an air container holding the Surround Background Tagger — a steel
 * H-beam supporting structure and LAB scintillator sensor cells forming a
 * 50 m rectangular frustum — wrapped around a central helium decay volume.
 * The SBT geometry is defined by the constants in SBTConstants.h.
 *
 * The helium is not an independent volume: it is derived from where the SBT
 * material actually is (see SBTEnvelope.h), so that it can neither overlap the
 * structure and sensors nor leave an unphysical margin behind.
 *
 * Placement: see descriptor().
 */
class DecayVolumeFactory {
   public:
    explicit DecayVolumeFactory(Materials& materials);
    ~DecayVolumeFactory() = default;

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * Integration layout 2026-1.2 (EDMS 3287817 v1.2): 50 m vessel centred at
     * z = 58.27 m (33.27 to 83.27 m). The container adds 0.2 m of air at each
     * end, so it spans 33.07 to 83.47 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        using units::m;
        return {.name = "DecayVolume",
                .node = "/SHiP/decay_volume",
                .id = 4,
                .x_mm = 0.0,
                .y_mm = 0.0,
                .z_mm = gm(58.27 * m),
                .isWorld = false};
    }
    /// Build the DecayVolume geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
