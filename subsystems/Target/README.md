# Target Subsystem

The Target subsystem contains the SHiP proton target and its associated shielding.

## Overview

The proton target is where the 400 GeV/c proton beam from the SPS interacts to produce
hidden sector particles. The target consists of tungsten slabs with tantalum cladding,
housed in a helium-filled vessel, surrounded by copper and iron shielding.

## Geometry Structure

```
target_vacuum_box (Vacuum, 160×227.1×300 cm)
├── proximity_shielding (Copper)
├── top_shielding (Copper, 160×60×300 cm)
├── bottom_shielding (Copper, 160×54.5×300 cm)
├── shielding_pedestal (Iron, 107×15×217 cm)
└── TargetArea
    ├── TargetVessel (Inconel718, tube r=20-20.8 cm, z=171.04 cm)
    ├── TargetVesselFront (Inconel718, disk r=20.8 cm, z=0.8 cm)
    ├── TargetVesselBack (Inconel718, disk r=20.8 cm, z=0.8 cm)
    └── HeVolume (PressurisedHe90, tube r=20 cm, z=171.04 cm)
        ├── target_enclosure (Steel316L)
        └── 19× CladdedTarget_N (Tantalum, r=12.5 cm)
            └── TargetCore_N (Tungsten, r=12.35 cm)
```

## Materials

| Component | Material | Density (g/cm³) |
|-----------|----------|-----------------|
| Vacuum box | Vacuum | 1.205e-6 |
| Shielding | Copper | 8.96 |
| Pedestal | Iron | 7.87 |
| Vessel | Inconel718 | 8.19 |
| Enclosure | Steel316L | 7.99 |
| He volume | PressurisedHe90 | 0.00212 |
| Cladding | Tantalum | 16.65 |
| Core | Tungsten | 19.3 |

## Target Slabs

The target consists of 19 tungsten slabs with tantalum cladding. The slab thicknesses
increase along the beam direction to account for the hadronic shower development.

| Slab | Cladding Z (cm) | Core Z (cm) | Position Z (cm) |
|------|-----------------|-------------|-----------------|
| 1 | 4.5 | 4.2 | -77.07 |
| 2 | 1.8 | 1.5 | -73.47 |
| 3 | 1.7 | 1.4 | -71.27 |
| 4 | 1.7 | 1.4 | -69.12 |
| 5 | 1.8 | 1.5 | -66.92 |
| 6 | 1.9 | 1.6 | -64.62 |
| 7 | 2.1 | 1.8 | -62.17 |
| 8 | 1.9 | 1.6 | -59.72 |
| 9 | 2.1 | 1.8 | -57.27 |
| 10 | 2.4 | 2.1 | -54.57 |
| 11 | 2.8 | 2.5 | -51.52 |
| 12 | 3.3 | 3.0 | -48.02 |
| 13 | 4.1 | 3.8 | -43.87 |
| 14 | 5.6 | 5.3 | -38.57 |
| 15 | 8.8 | 8.5 | -30.92 |
| 16 | 17.2 | 16.9 | -17.44 |
| 17 | 28.7 | 28.4 | +6.01 |
| 18 | 28.8 | 28.5 | +35.44 |
| 19 | 28.8 | 28.5 | +64.92 |

## Position in World

The target_vacuum_box is placed in the cave at position:
- X: 0 cm
- Y: -14.45 cm (below beam height)
- Z: 43.25 cm (downstream of origin)

## Usage

```cpp
#include "Target/TargetFactory.h"
#include "SHiPGeometry/SHiPMaterials.h"

SHiPMaterials materials;
TargetFactory factory(materials);
GeoPhysVol* target = factory.build();
```

## References

- GDML source: `gdml/ship_geometry.gdml`
- SHiP Technical Proposal: CERN-SPSC-2015-016
