// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

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
 * Z: 32.92 to 83.32 m -> centre 58.12 m; placement handled by SHiPGeometry.
 */
class DecayVolumeFactory {
   public:
    explicit DecayVolumeFactory(SHiPMaterials& materials);
    ~DecayVolumeFactory() = default;

    /// Build the DecayVolume geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;
};

}  // namespace SHiPGeometry
