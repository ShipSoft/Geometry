// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/NeutrinoDetector/NeutrinoDetectorFactory.h"
#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

#include <algorithm>
#include <array>
#include <cstddef>

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/// One muon-shield magnet: a solid iron box on the beam axis, from its
/// upstream face in the world frame. Declared outside MuonShieldFactory so
/// that its member functions can be used in the factory's constants.
struct MuonShieldMagnet {
    const char* name;
    units::LengthMm zStart;
    units::LengthMm halfX, halfY, halfZ;

    [[nodiscard]] constexpr units::LengthMm zEnd() const { return zStart + 2.0 * halfZ; }
    [[nodiscard]] constexpr units::LengthMm zCentre() const { return zStart + halfZ; }
};

/**
 * @brief Factory for the muon shield: the magnetised hadron stopper and the
 * shield magnets M1-M7.
 *
 * Solid-block approximation of the FairShip TRY_2026 shield (integration
 * layout 2026-0.1): the hadron stopper and each magnet are one solid iron box
 * of the magnet's outer size, as FairShip builds them. The apertures and yoke
 * shapes are not modelled (#125), and neither is the field, which comes from
 * the field map.
 *
 * The scattering and neutrino detector sits in the end of M5 and the whole of
 * M6, as in FairShip, so those two magnets have a cavity for it cut out of
 * their iron (k_sndCavity). The cavity is the SND container's shape at its
 * placement, so it steps with the SND's sub-detectors and the iron comes up
 * to each of them. Only NeutrinoDetectorFactory's header is used, for its
 * constants; the libraries do not depend on each other.
 */
class MuonShieldFactory {
   public:
    explicit MuonShieldFactory(Materials& materials);
    ~MuonShieldFactory() = default;

    [[nodiscard]] GeoPhysVol* build();

    using MagnetData = MuonShieldMagnet;

    // Source: FairShip python/geometry_config.py, shield_db["TRY_2026"], one
    // row [Zgap, dZ, dX, dY] in cm per magnet. The half-sizes are (dX, dY, dZ).
    // Each magnet starts its Zgap after the previous one ends, and the first
    // row starts at the hadron stopper's front face, 2.14 m (FairShip
    // hadronAbsorber.z, and the EDMS envelope).

    /// Row 0: the magnetised hadron stopper, [0, 115, 163.5, 169].
    static constexpr MagnetData k_hadronStopper{.name = "magn_absorb",
                                                .zStart = 2140.0 * units::mm,
                                                .halfX = 1635.0 * units::mm,
                                                .halfY = 1690.0 * units::mm,
                                                .halfZ = 1150.0 * units::mm};

    /// Rows 1-7: the shield magnets M1-M7.
    static constexpr std::array<MagnetData, 7> k_magnets{{
        // [15, 150, 136, 80]
        {.name = "magn_1",
         .zStart = 4590.0 * units::mm,
         .halfX = 1360.0 * units::mm,
         .halfY = 800.0 * units::mm,
         .halfZ = 1500.0 * units::mm},
        // [30, 225, 149, 89]
        {.name = "magn_2",
         .zStart = 7890.0 * units::mm,
         .halfX = 1490.0 * units::mm,
         .halfY = 890.0 * units::mm,
         .halfZ = 2250.0 * units::mm},
        // [16, 225, 155.2, 85]
        {.name = "magn_3",
         .zStart = 12550.0 * units::mm,
         .halfX = 1552.0 * units::mm,
         .halfY = 850.0 * units::mm,
         .halfZ = 2250.0 * units::mm},
        // [15, 225, 159.2, 60]
        {.name = "magn_4",
         .zStart = 17200.0 * units::mm,
         .halfX = 1592.0 * units::mm,
         .halfY = 600.0 * units::mm,
         .halfZ = 2250.0 * units::mm},
        // [16, 131, 134, 52]
        {.name = "magn_5",
         .zStart = 21860.0 * units::mm,
         .halfX = 1340.0 * units::mm,
         .halfY = 520.0 * units::mm,
         .halfZ = 1310.0 * units::mm},
        // [15, 168, 150, 63]
        {.name = "magn_6",
         .zStart = 24630.0 * units::mm,
         .halfX = 1500.0 * units::mm,
         .halfY = 630.0 * units::mm,
         .halfZ = 1680.0 * units::mm},
        // [19, 200, 171, 127]
        {.name = "magn_7",
         .zStart = 28180.0 * units::mm,
         .halfX = 1710.0 * units::mm,
         .halfY = 1270.0 * units::mm,
         .halfZ = 2000.0 * units::mm},
    }};

    /// The container spans the shield exactly: the hadron stopper's upstream
    /// face to M7's downstream face, as wide and tall as the widest and the
    /// tallest block.
    static constexpr auto s_zStart = k_hadronStopper.zStart;
    static constexpr auto s_zEnd = k_magnets.back().zEnd();
    static constexpr auto s_areaHalfX = std::ranges::max(
        k_hadronStopper.halfX, std::ranges::max(k_magnets, {}, &MagnetData::halfX).halfX);
    static constexpr auto s_areaHalfY = std::ranges::max(
        k_hadronStopper.halfY, std::ranges::max(k_magnets, {}, &MagnetData::halfY).halfY);
    static constexpr auto s_areaHalfZ = 0.5 * (s_zEnd - s_zStart);
    static constexpr auto s_centreZ = 0.5 * (s_zStart + s_zEnd);

    /// One step of the SND cavity, in the world frame: a box on the beam axis
    /// from @c zStart to @c zEnd.
    struct CavitySection {
        units::LengthMm zStart, zEnd;
        units::LengthMm halfX, halfY;
    };

    /// Cavity for the SND (veto, target, MGC, HCAL): the SND container's
    /// sections (NeutrinoDetectorFactory::k_sections) placed where the SND
    /// is. The .cpp checks that it cuts M5 and M6 only and that the HCAL step
    /// is exactly M6.
    static constexpr auto k_sndCavity = [] {
        using NDF = NeutrinoDetectorFactory;
        static_assert(NDF::descriptor().x_mm == 0.0 && NDF::descriptor().y_mm == 0.0,
                      "the SND cavity assumes the SND is centred on the beam axis");
        const auto origin = NDF::descriptor().z_mm * units::mm;
        std::array<CavitySection, NDF::k_sections.size()> cavity{};
        for (std::size_t i = 0; i < cavity.size(); ++i) {
            const auto& s = NDF::k_sections.at(i);
            cavity.at(i) = {.zStart = origin + s.zStart,
                            .zEnd = origin + s.zEnd,
                            .halfX = s.halfX,
                            .halfY = s.halfY};
        }
        return cavity;
    }();

    /// The cavity section overlaps the magnet's iron along z.
    [[nodiscard]] static constexpr bool cuts(const CavitySection& section,
                                             const MagnetData& magnet) {
        return magnet.zStart < section.zEnd && section.zStart < magnet.zEnd();
    }

    /// Some of the SND cavity lies in this magnet, so it is cut out.
    [[nodiscard]] static constexpr bool hasSndCavity(const MagnetData& magnet) {
        return std::ranges::any_of(k_sndCavity,
                                   [&magnet](const CavitySection& s) { return cuts(s, magnet); });
    }

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * The container is centred between the hadron stopper's upstream face
     * (2.14 m) and M7's downstream face (32.18 m), at 17.16 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        return {.name = "MuonShield",
                .node = "/SHiP/muon_shield",
                .id = 2,
                .x_mm = 0.0,
                .y_mm = 0.0,
                .z_mm = gm(s_centreZ),
                .isWorld = false};
    }

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
