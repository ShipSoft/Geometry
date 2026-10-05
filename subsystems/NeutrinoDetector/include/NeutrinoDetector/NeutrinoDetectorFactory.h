// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "NeutrinoDetector/SNDEnvelope.h"

class GeoPhysVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Scattering and Neutrino Detector (SND).
 *
 * Builds the SND as a single air container holding three sub-detectors in
 * sequence along the beam (upstream → downstream):
 *
 *   1. Veto      — an upstream PVT scintillator-bar station: three planes of
 *                  seven bars (two horizontal, staggered in Y; one vertical).
 *   2. NuTarget  — a silicon/tungsten sampling target: 120 layers, each a
 *                  tungsten absorber plate followed by an X and a Y silicon
 *                  tracking plane (400 mm reference plate, 11.3 mm pitch).
 *   3. HCAL      — a hadronic calorimeter: three transverse sections
 *                  (40/50/60 cm), 14 layers each; every layer is an iron
 *                  absorber, an X and a Y scintillating-fibre plane, and a
 *                  polystyrene tile grid.
 *
 * The geometry follows the repo factory idiom (calorimeter / straw tracker):
 * a `/SHiP/neutrino_detector` air container filled with hierarchically named
 * `GeoPhysVol` children that reuse shared `GeoLogVol`s; sensitive-detector
 * assignment is done downstream by name.
 *
 * Each 0.5 mm scintillating-fibre plane is built from its individual fibres,
 * two staggered sub-layers of 0.25 mm fibres. To keep the node count bounded,
 * the O(300k) identical fibres are not placed one node at a time: a single
 * shared fibre `GeoPhysVol` is multiply placed per sub-layer via a
 * `GeoSerialTransformer`, so each plane costs O(1) tree nodes regardless of
 * fibre count. The X/Y plane names are preserved for readout.
 *
 * The container is a box sized from the SD.toml reservation envelope
 * (`size`), the same box that is carved out of the muon-shield iron, so the
 * container always matches its cavity. Placement is handled by SHiPGeometry.
 */
class NeutrinoDetectorFactory {
   public:
    /// Construct the factory against the shared materials catalogue. The
    /// container size is taken from @p envelope (by default, read from SD.toml).
    explicit NeutrinoDetectorFactory(SHiPMaterials& materials,
                                     const SNDEnvelope& envelope = readSNDEnvelope());

    /// Defaulted destructor.
    ~NeutrinoDetectorFactory() = default;

    /// Build the SND geometry; returns the air container.
    /// @throws std::runtime_error if the contents do not fit in the container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // ── Container envelope (mm) ─────────────────────────────────────────
    // Half of the SD.toml envelope `size`, the same box that is carved out of
    // the muon-shield iron.
    double m_halfX;
    double m_halfY;
    double m_halfZ;
};

}  // namespace SHiPGeometry
