// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include "SHiP/geometry/SubsystemDescriptor.h"
#include "SHiP/geometry/Units.h"

#include <array>

class GeoPhysVol;

namespace SHiP::geometry {

class Materials;

/**
 * @brief Factory for the MuonShield (muon shield magnets) geometry
 *
 * Solid-block approximation of the FairShip TRY_2026 shield (integration
 * layout 2026-0.1): magnets M1-M7, each one solid iron box of the magnet's
 * outer size, as FairShip builds them. The aperture, yoke shape and field are
 * not modelled; the field comes from the field map. The hadron stopper
 * (FairShip row 0) belongs to the Target subsystem.
 *
 * The scattering and neutrino detector sits in the end of M5 and the whole of
 * M6, as in FairShip, so those two magnets have a cavity for it cut out of
 * their iron (k_sndCavity). The cavity steps with the SND's sub-detectors, so
 * the iron comes up to each of them; Layout.h checks that it matches the SND
 * container section by section.
 */
class MuonShieldFactory {
   public:
    explicit MuonShieldFactory(Materials& materials);
    ~MuonShieldFactory() = default;

    [[nodiscard]] GeoPhysVol* build();

    /// One magnet: a solid iron box, from its upstream face in the world frame.
    struct MagnetData {
        const char* name;
        units::LengthMm zStart;
        units::LengthMm halfX, halfY, halfZ;
    };

    /// FairShip TRY_2026 magnets (python/geometry_config.py, shield_db): the
    /// half-sizes are its (dX, dY, dZ), and each magnet starts its Zgap after
    /// the previous one ends.
    static constexpr std::array<MagnetData, 7> k_magnets{{
        {"magn_1", 4590.0 * units::mm, 1360.0 * units::mm, 800.0 * units::mm, 1500.0 * units::mm},
        {"magn_2", 7890.0 * units::mm, 1490.0 * units::mm, 890.0 * units::mm, 2250.0 * units::mm},
        {"magn_3", 12550.0 * units::mm, 1552.0 * units::mm, 850.0 * units::mm, 2250.0 * units::mm},
        {"magn_4", 17200.0 * units::mm, 1592.0 * units::mm, 600.0 * units::mm, 2250.0 * units::mm},
        {"magn_5", 21860.0 * units::mm, 1340.0 * units::mm, 520.0 * units::mm, 1310.0 * units::mm},
        {"magn_6", 24630.0 * units::mm, 1500.0 * units::mm, 630.0 * units::mm, 1680.0 * units::mm},
        {"magn_7", 28180.0 * units::mm, 1710.0 * units::mm, 1270.0 * units::mm, 2000.0 * units::mm},
    }};

    /// The container spans the magnets exactly: M1's upstream face to M7's
    /// downstream face, and the widest and tallest magnet (M7).
    static constexpr auto s_zStart = 4590.0 * units::mm;
    static constexpr auto s_zEnd = 32180.0 * units::mm;
    static constexpr auto s_areaHalfX = 1710.0 * units::mm;
    static constexpr auto s_areaHalfY = 1270.0 * units::mm;
    static constexpr auto s_areaHalfZ = 0.5 * (s_zEnd - s_zStart);
    static constexpr auto s_centreZ = 0.5 * (s_zStart + s_zEnd);

    /// One step of the SND cavity, in the world frame: a box on the beam axis
    /// from @c zStart to @c zEnd.
    struct CavitySection {
        units::LengthMm zStart, zEnd;
        units::LengthMm halfX, halfY;
    };

    /// Cavity for the SND (veto, target, MGC, HCAL), the same boxes as the SND
    /// container's sections (NeutrinoDetectorFactory::k_sections, checked in
    /// Layout.h). It ends at M6's downstream face, and the HCAL step is
    /// exactly M6, so M6 keeps iron right around the HCAL.
    static constexpr std::array<CavitySection, 4> k_sndCavity{{
        {22940.0 * units::mm, 22980.0 * units::mm, 211.0 * units::mm, 220.0 * units::mm},
        {22980.0 * units::mm, 24420.0 * units::mm, 201.0 * units::mm, 201.0 * units::mm},
        {24420.0 * units::mm, 24630.0 * units::mm, 251.0 * units::mm, 251.0 * units::mm},
        {24630.0 * units::mm, 27990.0 * units::mm, 301.25 * units::mm, 301.25 * units::mm},
    }};

    /**
     * @brief This subsystem's self-description (name, node, id, placement).
     *
     * The container is centred between M1's upstream face (4.59 m) and M7's
     * downstream face (32.18 m), at 18.385 m.
     */
    static constexpr SubsystemDescriptor descriptor() {
        using units::gm;
        return {"MuonShield", "/SHiP/muon_shield", 2, 0.0, 0.0, gm(s_centreZ), false};
    }

    /// The cavity section overlaps the magnet's iron along z.
    [[nodiscard]] static constexpr bool cuts(const CavitySection& section,
                                             const MagnetData& magnet) {
        return magnet.zStart < section.zEnd && section.zStart < magnet.zStart + 2 * magnet.halfZ;
    }

    /// Some of the SND cavity lies in this magnet, so it is cut out.
    [[nodiscard]] static constexpr bool hasSndHole(const MagnetData& magnet) {
        for (const auto& section : k_sndCavity) {
            if (cuts(section, magnet)) {
                return true;
            }
        }
        return false;
    }

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
