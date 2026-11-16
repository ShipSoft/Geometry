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
 * M6, as in FairShip, so those two magnets have the SND box (s_sndHole*) cut
 * out of their iron. Layout.h checks that the SND container fills that hole.
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

    /// Hole for the SND, in the world frame: it ends at M6's downstream face
    /// and is as long as the SND container (5.1 m), so it starts in M5.
    static constexpr auto s_sndHoleHalfX = 400.0 * units::mm;
    static constexpr auto s_sndHoleHalfY = 400.0 * units::mm;
    static constexpr auto s_sndHoleZEnd = 27990.0 * units::mm;
    static constexpr auto s_sndHoleZStart = s_sndHoleZEnd - 5100.0 * units::mm;

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

    /// The magnet's iron overlaps the SND hole along z, so the hole is cut out.
    [[nodiscard]] static constexpr bool hasSndHole(const MagnetData& magnet) {
        return magnet.zStart < s_sndHoleZEnd && s_sndHoleZStart < magnet.zStart + 2 * magnet.halfZ;
    }

   private:
    Materials& m_materials;
};

}  // namespace SHiP::geometry
