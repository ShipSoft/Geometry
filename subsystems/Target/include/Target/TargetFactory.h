// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/Units.h>

#include <array>

class GeoPhysVol;
class GeoLogVol;

namespace SHiPGeometry {

class SHiPMaterials;

/**
 * @brief Factory for the Target (proton target and shielding) geometry
 *
 * The target consists of:
 * - Vacuum box containing shielding and target assembly
 * - Copper proximity, top, and bottom shielding
 * - Iron shielding pedestal
 * - Inconel718 target vessel with helium-filled interior
 * - 19 tungsten target slabs with tantalum cladding
 */
class TargetFactory {
   public:
    explicit TargetFactory(SHiPMaterials& materials);
    ~TargetFactory() = default;

    /**
     * @brief Build the Target geometry
     * @return Pointer to the target_vacuum_box physical volume
     */
    GeoPhysVol* build();

   private:
    SHiPMaterials& m_materials;

    // Helper methods
    GeoPhysVol* createProximityShielding();
    GeoPhysVol* createTopShielding();
    GeoPhysVol* createBottomShielding();
    GeoPhysVol* createShieldingPedestal();
    GeoPhysVol* createTargetVessel();
    GeoPhysVol* createTargetVesselFront();
    GeoPhysVol* createTargetVesselBack();
    GeoPhysVol* createTargetEnclosure();
    GeoPhysVol* createHeVolume();

    // Unit conversion helper (GDML uses cm, GeoModel uses mm)
    static constexpr double cm = GeoModelKernelUnits::cm;

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

    // TargetArea position within vacuum box
    static constexpr double s_targetAreaPosY = 14.45 * cm;
    static constexpr double s_targetAreaPosZ = -43.25 * cm;

    // Target vessel
    static constexpr double s_vesselRmin = 20.0 * cm;
    static constexpr double s_vesselRmax = 20.8 * cm;
    static constexpr double s_vesselHalfZ = 85.52 * cm;
    static constexpr double s_vesselPosZ = 79.32 * cm;

    // Target vessel end caps
    static constexpr double s_vesselCapRadius = 20.8 * cm;
    static constexpr double s_vesselCapHalfZ = 0.4 * cm;
    static constexpr double s_vesselFrontPosZ = -6.6 * cm;
    static constexpr double s_vesselBackPosZ = 165.24 * cm;

    // HeVolume
    static constexpr double s_heVolumeRadius = 20.0 * cm;
    static constexpr double s_heVolumeHalfZ = 85.52 * cm;
    static constexpr double s_heVolumePosZ = 79.32 * cm;

    // Target enclosure
    static constexpr double s_enclosureRmin = 12.51 * cm;
    static constexpr double s_enclosureRmax = 19.2 * cm;
    static constexpr double s_enclosureHalfZ = 79.32 * cm;
    static constexpr double s_enclosureCutoutHalfX = 8.0 * cm;
    static constexpr double s_enclosureCutoutHalfY = 14.0 * cm;

    // Target slabs (common parameters)
    static constexpr double s_claddingRadius = 12.5 * cm;
    static constexpr double s_coreRadius = 12.35 * cm;
    static constexpr int s_numSlabs = 19;

    // Target slab data arrays (half-lengths in Z, positions in Z)
    static constexpr std::array<double, 19> s_claddingHalfZ = {
        2.25 * cm, 0.9 * cm,  0.85 * cm,  0.85 * cm, 0.9 * cm,  0.95 * cm, 1.05 * cm,
        0.95 * cm, 1.05 * cm, 1.2 * cm,   1.4 * cm,  1.65 * cm, 2.05 * cm, 2.8 * cm,
        4.4 * cm,  8.6 * cm,  14.35 * cm, 14.4 * cm, 14.4 * cm};
    static constexpr std::array<double, 19> s_coreHalfZ = {
        2.1 * cm,  0.75 * cm, 0.7 * cm,  0.7 * cm,   0.75 * cm, 0.8 * cm, 0.9 * cm,
        0.8 * cm,  0.9 * cm,  1.05 * cm, 1.25 * cm,  1.5 * cm,  1.9 * cm, 2.65 * cm,
        4.25 * cm, 8.45 * cm, 14.2 * cm, 14.25 * cm, 14.25 * cm};
    static constexpr std::array<double, 19> s_slabPosZ = {
        -77.07 * cm, -73.47 * cm, -71.27 * cm, -69.12 * cm, -66.92 * cm, -64.62 * cm, -62.17 * cm,
        -59.72 * cm, -57.27 * cm, -54.57 * cm, -51.52 * cm, -48.02 * cm, -43.87 * cm, -38.57 * cm,
        -30.92 * cm, -17.44 * cm, 6.01 * cm,   35.44 * cm,  64.92 * cm};
};

}  // namespace SHiPGeometry
