// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

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
 * Approximation: each 0.5 mm scintillating-fibre plane (two staggered
 * sub-layers of 0.25 mm fibres in the original standalone simulation) is
 * modelled here as a single homogeneous polystyrene slab of the same
 * thickness. Tightly-packed 0.25 mm fibres are ~solid polystyrene, and
 * placing them individually would add O(300k) volumes — far beyond the scale
 * of any other subsystem. The X/Y plane names are preserved for readout.
 *
 * The container is a box approximation of the SND envelope
 * (`subsystem_envelopes.csv`: z 26.40–31.50 m WARM, half-width/height up to
 * 0.40 m); placement is handled by SHiPGeometry.
 */
class NeutrinoDetectorFactory {
   public:
    /// Construct the factory against the shared materials catalogue.
    explicit NeutrinoDetectorFactory(SHiPMaterials& materials);

    /// Defaulted destructor.
    ~NeutrinoDetectorFactory() = default;

    /// Build the SND geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // ── Container envelope (mm) ─────────────────────────────────────────
    // Box approximation of the frustum SND envelope, sized to the largest
    // (downstream) half-width/height and the full 5.10 m length.
    static constexpr double s_halfX = 400.0;
    static constexpr double s_halfY = 400.0;
    static constexpr double s_halfZ = 2550.0;
};

}  // namespace SHiPGeometry
