# Target Subsystem

The Target subsystem contains the SHiP proton target and its associated shielding.

## Overview

The proton target is where the 400 GeV/c proton beam from the SPS interacts to produce
hidden sector particles. This implements the 2026 BDF target design, extracted from
CATIA model `ST1A07710_01_AB.02`: solid tungsten disks held in a steel core with
serpentine helium cooling grooves, surrounded by a steel jacket with flanges and a
domed rear endcap, all inside a pressurised-helium container. Copper and iron
shielding surround the target.

## Geometry Structure

```
target_vacuum_box (Vacuum, 160×227.1×300 cm)
├── proximity_shielding (Copper)
├── top_shielding (Copper, 160×60×300 cm)
├── bottom_shielding (Copper, 160×54.5×300 cm)
├── shielding_pedestal (Iron, 107×15×217 cm)
├── he_volume (PressurisedHe90, tube r=23.7 cm, z=-3.78..150.97 cm)
│   ├── 33× core_N (Tungsten disks, r=12.5 cm; core_33 r=15.7 cm)
│   ├── core_steel (Steel316L, polycone with He grooves subtracted)
│   ├── jacket (Steel316L, tube r=21.7-22.5 cm)
│   ├── flange_front (Steel316L, r=19.5-22.5 cm)
│   ├── front_window (Steel316L, disc r=14.1 cm, 8 mm)
│   ├── front_nose (Steel316L, r=14.1-19.5 cm)
│   ├── cover_ring1, cover_ring2 (Steel316L, cover-plate bore rings)
│   ├── flange_back (Steel316L, r=22.5-23.7 cm)
│   └── endcap (Steel316L, 8 mm domed shell as polycone)
└── cover_plate (Steel316L, 60×65×2 cm plate minus the He container bore)
```

All target z coordinates are measured from the front face of the first disk, which is
the SHiP global origin.

## Tungsten disks

33 solid tungsten disks (no cladding), radius 125 mm except the 455 mm rear block
(radius 157 mm), separated by 4 mm helium slits (5 mm after disks 25 and 26). Total
length 1460 mm, of which 1330 mm tungsten.

Thickness sequence (mm): 30, 10, 15×8, 9, 11, 12, 13, 13, 16, 20, 20, 21, 30, 40,
55, 73, 106, 276, 455.

## Steel core, cooling grooves, jacket and endcap

- **Core** (z 7.2–1443.7 mm): the two clamp halves of the CAD model merged into one
  axisymmetric polycone; bore r=125 mm (stepping to 157 mm at z=1005 mm around the
  rear block), outer r=207 mm with a front step to r=195 mm (inside the front flange)
  and a rear step to r=190 mm.
- **He cooling grooves**: 61°-wide arcs (r 125→153 mm) in the bore, centred on the
  vertical axis and staggered in z between the upper half (12.2–57, 95–153, 191–257,
  300–464, 559–933.2 mm) and the lower half (45–104.7, 142.7–200.7, 238.7–334.7,
  385.7–1005 mm), plus a 49°-wide top groove (r 157→182 mm, z 1005–1441.7 mm) around
  the rear block. Together with the inter-disk slits they form the serpentine helium
  flow path. Implemented as tube-segment subtractions; the removed volume fills with
  the parent helium.
- **Jacket**: 8 mm steel tube (r 217–225 mm, z 55.2–1198.7 mm) with front flange
  (r 195–225 mm, z −23.8–55.2 mm) and rear flange (r 225–237 mm, z 1198.7–1263.7 mm);
  the r 207–217 mm gap between core and jacket is a helium annulus.
- **Upstream closure**: the vessel bore is closed by a dished beam window integral to
  the front flange part, simplified to a flat disc (r 141 mm, z −33.5–−25.5 mm) that
  preserves the 8 mm of steel on the beam axis; a nose ring (r 141–195 mm,
  z −35.8–−23.8 mm) connects the window rim to the flange. An external cover plate
  (600×650×20 mm, z −37.8–−17.8 mm, stepped bore r 196/226 mm, asymmetric about the
  beam axis: +250 mm above, −400 mm below) is modelled as two bore rings inside the
  helium container plus a rectangular box-minus-tube outside it.
- **Rear endcap**: 8 mm shell (cylinder r 229–237 mm, z 1263.7–1413.7 mm) closed by a
  domed head approximated as a polycone (three cone segments and an apex disc,
  ending at z=1509.7 mm).

Simplifications relative to the CAD model: bolt lugs, key-ways, pipe stubs, fasteners
and survey hardware are omitted; the flange/jacket boundary is placed at the end of
the core front step (55.2 mm instead of the CAD's stepped flange bore at 64.2 mm); the
beam window dishing is dropped and the window spacer pads are omitted.

## Materials

| Component | Material | Density (g/cm³) |
|-----------|----------|-----------------|
| Vacuum box | Vacuum | 1.205e-6 |
| Shielding | Copper | 8.96 |
| Pedestal | Iron | 7.87 |
| Disks | Tungsten | 19.3 |
| Core, jacket, flanges, window, cover plate, endcap | Steel316L | 7.99 |
| He volume | PressurisedHe90 | 0.00212 |

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

- CATIA model: `ST1A07710_01_AB.02` (July 2026)
- FairShip implementation of the same design: ShipSoft/FairShip#1361
- SHiP Technical Proposal: CERN-SPSC-2015-016
