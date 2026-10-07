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
     * Z: 32.92 to 83.32 m, so the container is centred at z = 58.12 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        using units::m;
        return {"DecayVolume", "/SHiP/decay_volume", 4, 0.0, 0.0, gm(58.12 * m), false};
    }
    /// Build the DecayVolume geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
