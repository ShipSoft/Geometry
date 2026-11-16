// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the Scattering and Neutrino Detector (SND).
 *
 * Builds the SND as a single air container holding four sub-detectors in
 * sequence along the beam (upstream → downstream):
 *
 *   1. Veto      — an upstream PVT scintillator-bar station: three planes of
 *                  seven bars (two horizontal, staggered in Y; one vertical).
 *   2. NuTarget (HGC)  — a silicon/tungsten sampling target: 120 layers, each a
 *                  tungsten absorber plate followed by an X and a Y silicon
 *                  tracking plane (400 mm reference plate, 11.3 mm pitch).
 *                  3 Timing layers are inserted at 1/3, 2/3 and 3/3 of the target depth.
 *   3. MGC        — a medium-granularity calorimeter: 10 layers, each a tungsten
 *                  absorber plate followed by an X and a Y silicon tracking plane.
 *   4. HCAL (MTC)   — a hadronic calorimeter: three transverse sections
 *                  (60/60/60 cm), 14 layers each; every layer is an iron
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
 * The container is a box of the SND envelope's cross-section
 * (`subsystem_envelopes.csv`: half-width/height up to 0.40 m), 5.1 m long;
 * placement is handled by DetectorBuilder.
 */
class NeutrinoDetectorFactory {
   public:
    /// Construct the factory against the shared materials catalogue.
    explicit NeutrinoDetectorFactory(Materials& materials);

    /// Defaulted destructor.
    ~NeutrinoDetectorFactory() = default;

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * Z: 22.89 to 27.99 m, so the container is centred at z = 25.44 m. As in
     * FairShip's TRY_2026 layout, the SND sits in the end of muon-shield magnet
     * M5 and the whole of M6, which have a matching hole cut out of their iron
     * (MuonShieldFactory, checked in Layout.h). This is 3.51 m upstream of the
     * EDMS envelope (26.40-31.50 m), which follows the WARM shield layout.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        using units::m;
        return {"NeutrinoDetector", "/SHiP/neutrino_detector", 9, 0.0, 0.0, gm(25.44 * m), false};
    }
    /// Build the SND geometry; returns the air container.
    [[nodiscard]] GeoPhysVol* build();

    // ── Container envelope ──────────────────────────────────────────────
    // Box approximation of the frustum SND envelope, sized to the largest
    // (downstream) half-width/height and the full 5.10 m length.
    static constexpr auto s_halfX = 400.0 * units::mm;
    static constexpr auto s_halfY = 400.0 * units::mm;
    static constexpr auto s_halfZ = 2550.0 * units::mm;

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
