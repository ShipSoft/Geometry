// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

#include <array>

class GeoPhysVol;
class GeoLogVol;
class GeoShape;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Target (proton target and shielding) geometry
 *
 * Implements the 2026 BDF target design, extracted from CATIA model
 * ST1A07710_01_AB.02 and simplified to axisymmetric shapes:
 * - Vacuum box containing shielding and target assembly
 * - Copper proximity, top, and bottom shielding
 * - Iron shielding pedestal
 * - Helium container (HeVolume) holding:
 *   - 33 solid tungsten disks (no cladding); the last disk (rear block)
 *     has a larger radius
 *   - steel core (the two clamp halves modelled as one axisymmetric
 *     polycone) with serpentine He cooling grooves subtracted
 *   - steel jacket tube with front and rear flanges; the gap between
 *     core and jacket is a He annulus
 *   - domed rear endcap (8 mm shell, dome approximated as a polycone)
 *
 * All target z coordinates are measured from the front face of the first
 * disk, which is the SHiP global origin.
 */
class TargetFactory {
   public:
    explicit TargetFactory(SHiPMaterials& materials);
    ~TargetFactory() = default;

    /**
     * @brief Build the Target geometry
     * @return Pointer to the target_vacuum_box physical volume
     */
    [[nodiscard]] GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // Helper methods
    GeoPhysVol* createProximityShielding();
    GeoPhysVol* createTopShielding();
    GeoPhysVol* createBottomShielding();
    GeoPhysVol* createShieldingPedestal();
    GeoPhysVol* createHeVolume();
    const GeoShape* createSteelCoreShape();

    // Unit conversion helpers
    static constexpr double cm = GeoModelKernelUnits::cm;
    static constexpr double mm = GeoModelKernelUnits::mm;
    static constexpr double deg = GeoModelKernelUnits::degree;

    // Target vacuum box dimensions (half-sizes)
    static constexpr double s_vacuumBoxHalfX = 80.0 * cm;
    static constexpr double s_vacuumBoxHalfY = 113.55 * cm;
    static constexpr double s_vacuumBoxHalfZ = 150.0 * cm;

    // Proximity shielding
    static constexpr double s_proxEnvHalfX = 80.0 * cm;
    static constexpr double s_proxEnvHalfY = 56.3 * cm;
    static constexpr double s_proxEnvHalfZ = 150.0 * cm;
    static constexpr double s_proxInnerHalfX = 55.0 * cm;
    static constexpr double s_proxInnerHalfY = 56.3 * cm;
    static constexpr double s_proxInnerHalfZ = 110.0 * cm;
    static constexpr double s_proxInnerOffsetZ = 15.0 * cm;
    static constexpr double s_proxHoleRadius = 10.0 * cm;
    static constexpr double s_proxHoleHalfZ = 27.5 * cm;
    static constexpr double s_proxHoleOffsetY = 17.2 * cm;
    static constexpr double s_proxHoleOffsetZ = -122.5 * cm;
    static constexpr double s_proxPosY = -2.75 * cm;

    // Top shielding
    static constexpr double s_topShieldHalfX = 80.0 * cm;
    static constexpr double s_topShieldHalfY = 30.0 * cm;
    static constexpr double s_topShieldHalfZ = 150.0 * cm;
    static constexpr double s_topShieldPosY = 83.55 * cm;

    // Bottom shielding
    static constexpr double s_bottomShieldHalfX = 80.0 * cm;
    static constexpr double s_bottomShieldHalfY = 27.25 * cm;
    static constexpr double s_bottomShieldHalfZ = 150.0 * cm;
    static constexpr double s_bottomShieldPosY = -86.3 * cm;

    // Shielding pedestal
    static constexpr double s_pedestalHalfX = 53.5 * cm;
    static constexpr double s_pedestalHalfY = 7.5 * cm;
    static constexpr double s_pedestalHalfZ = 108.5 * cm;
    static constexpr double s_pedestalPosY = -51.55 * cm;
    static constexpr double s_pedestalPosZ = 15.0 * cm;

    // TargetArea position within vacuum box: the target frame (z = 0 at the
    // front face of the first disk) sits at this offset in the vacuum box
    static constexpr double s_targetAreaPosY = 14.45 * cm;
    static constexpr double s_targetAreaPosZ = -43.25 * cm;

    // ---- 2026 BDF target (CATIA ST1A07710_01_AB.02) ----
    // All z values below are in the target frame (z = 0 at disk-1 front face).

    // HeVolume: container for disks, steel core, jacket, flanges and endcap
    static constexpr double s_heRadius = 237.0 * mm;
    static constexpr double s_heZMin = -35.8 * mm;   // upstream face of front flange
    static constexpr double s_heZMax = 1509.7 * mm;  // downstream end of rear endcap

    // Tungsten disks: [z start, z end] per disk; 4-5 mm He slits in between
    static constexpr int s_numDisks = 33;
    static constexpr double s_diskRadius = 125.0 * mm;
    static constexpr double s_lastDiskRadius = 157.0 * mm;  // rear block
    static constexpr std::array<std::array<double, 2>, 33> s_diskZ = {{
        {0.0 * mm, 30.0 * mm},    {34.0 * mm, 44.0 * mm},    {48.0 * mm, 56.0 * mm},
        {60.0 * mm, 68.0 * mm},   {72.0 * mm, 80.0 * mm},    {84.0 * mm, 92.0 * mm},
        {96.0 * mm, 104.0 * mm},  {108.0 * mm, 116.0 * mm},  {120.0 * mm, 128.0 * mm},
        {132.0 * mm, 140.0 * mm}, {144.0 * mm, 152.0 * mm},  {156.0 * mm, 164.0 * mm},
        {168.0 * mm, 176.0 * mm}, {180.0 * mm, 188.0 * mm},  {192.0 * mm, 200.0 * mm},
        {204.0 * mm, 212.0 * mm}, {216.0 * mm, 224.0 * mm},  {228.0 * mm, 237.0 * mm},
        {241.0 * mm, 252.0 * mm}, {256.0 * mm, 268.0 * mm},  {272.0 * mm, 285.0 * mm},
        {289.0 * mm, 302.0 * mm}, {306.0 * mm, 322.0 * mm},  {326.0 * mm, 346.0 * mm},
        {350.0 * mm, 370.0 * mm}, {375.0 * mm, 396.0 * mm},  {401.0 * mm, 431.0 * mm},
        {435.0 * mm, 475.0 * mm}, {479.0 * mm, 534.0 * mm},  {538.0 * mm, 611.0 * mm},
        {615.0 * mm, 721.0 * mm}, {725.0 * mm, 1001.0 * mm}, {1005.0 * mm, 1460.0 * mm},
    }};

    // Steel core (two clamp halves modelled as one axisymmetric piece)
    static constexpr double s_coreZMin = 7.2 * mm;
    static constexpr double s_coreZMax = 1443.7 * mm;
    static constexpr double s_coreBoreR1 = 125.0 * mm;      // matches disks 1..32
    static constexpr double s_coreBoreR2 = 157.0 * mm;      // matches the rear block
    static constexpr double s_coreBoreStepZ = 1005.0 * mm;  // bore radius change
    static constexpr double s_coreOuterR = 207.0 * mm;
    static constexpr double s_coreFrontOuterR = 195.0 * mm;  // front, inside the flange
    static constexpr double s_coreFrontZMax = 55.2 * mm;     // end of front step
    static constexpr double s_coreRearOuterR = 190.0 * mm;   // rear, inside rear flange
    static constexpr double s_coreRearZMin = 1233.7 * mm;    // start of rear step

    // Serpentine He cooling grooves: arcs in the bore, centred on the vertical
    // axis, staggered between the upper and lower half; He flows between them
    // through the inter-disk slits
    static constexpr double s_grooveRmin = 120.0 * mm;  // below bore: clean subtraction
    static constexpr double s_grooveRmax = 153.0 * mm;
    static constexpr double s_groovePhiWidth = 61.0 * deg;
    static constexpr std::array<std::array<double, 2>, 5> s_groovesTop = {{
        {12.2 * mm, 57.0 * mm},
        {95.0 * mm, 153.0 * mm},
        {191.0 * mm, 257.0 * mm},
        {300.0 * mm, 464.0 * mm},
        {559.0 * mm, 933.2 * mm},
    }};
    static constexpr std::array<std::array<double, 2>, 4> s_groovesBottom = {{
        {45.0 * mm, 104.7 * mm},
        {142.7 * mm, 200.7 * mm},
        {238.7 * mm, 334.7 * mm},
        {385.7 * mm, 1005.0 * mm},
    }};
    // Groove around the rear block (upper half only), ends 2 mm before the
    // core rear face
    static constexpr double s_rearGrooveRmin = 152.0 * mm;  // below bore R2
    static constexpr double s_rearGrooveRmax = 182.0 * mm;
    static constexpr double s_rearGroovePhiWidth = 49.0 * deg;
    static constexpr double s_rearGrooveZMin = 1005.0 * mm;
    static constexpr double s_rearGrooveZMax = 1441.7 * mm;

    // Jacket tube and flanges; He annulus between core (207) and jacket (217).
    // The flange bore matches the core front step, so the flange/jacket
    // boundary is placed at the end of the step.
    static constexpr double s_jacketRmin = 217.0 * mm;
    static constexpr double s_jacketRmax = 225.0 * mm;
    static constexpr double s_jacketZMin = s_coreFrontZMax;
    static constexpr double s_jacketZMax = 1198.7 * mm;
    static constexpr double s_flangeFrontRmin = 195.0 * mm;
    static constexpr double s_flangeRearRmax = 237.0 * mm;
    static constexpr double s_flangeRearZMax = 1263.7 * mm;

    // Rear endcap: 8 mm shell, cylindrical section closed by a domed head;
    // polycone planes (z, rmin, rmax) approximating the dome with three cone
    // segments and an apex disc
    static constexpr std::array<std::array<double, 3>, 6> s_endcapPlanes = {{
        {1263.7 * mm, 229.0 * mm, 237.0 * mm},
        {1413.7 * mm, 229.0 * mm, 237.0 * mm},
        {1450.0 * mm, 212.0 * mm, 223.0 * mm},
        {1480.0 * mm, 142.0 * mm, 166.5 * mm},
        {1505.0 * mm, 0.0 * mm, 67.0 * mm},
        {1509.7 * mm, 0.0 * mm, 67.0 * mm},
    }};
};

}  // namespace SHiPGeometry
