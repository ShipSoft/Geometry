# MuonShield

Passive iron muon shield for background suppression.

## Description

The MuonShield subsystem reserves an Air **envelope** and places an explicit
list of iron **blocks** inside it. Each block is a box, or a symmetric
trapezoid/frustum (`GeoTrd`) when tapered, anchored by its upstream face and
optionally rotated. The block list is the sole source of iron: holes and
apertures are represented by leaving a region free of blocks, or by arranging
several blocks around the gap.

The default `muon_shield.toml` is the **FairShip TRY_2026** magnet layout (FairShip
PR #1334) as a solid-block approximation: the 7 shield magnets, each reproduced
as one iron block from its outer envelope (aperture, yoke and field dropped).
Each row of the FairShip `ShipMuonShield` `params` table becomes one block, its
transverse size taken as `dX*(1+ratio) + midGap + ceil(max(100/dY, gap))` by
`dY + dY_yoke`.

The FairShip absorber (row 0) is the magnetised hadron stopper. It is not a
shield magnet: the Target subsystem builds it as `/SHiP/hadron_stopper` over
z = 2.14–4.44 m, so it is absent here and the shield envelope starts downstream
of it at 4.54 m.

## Geometry Tree

The default `muon_shield.toml` places the 7 TRY_2026 magnets as solid blocks. The SND
reservation (`NeutrinoDetector/SD.toml`) is subtracted from the iron at build
time, and the SND is embedded as a daughter that sits in the resulting cavity:

```text
muon_shield (Air envelope, 3520 × 2640 × 27540 mm)   centre z = 18.31 m
 ├─ block_0 … block_4   magnets 1–5 (Iron boxes)
 ├─ block_5             magnet 6 (Iron − SND box)   ┐ carved by A − B
 ├─ block_6             magnet 7 (Iron − SND box)   ┘ (magnets the box intersects)
 └─ neutrino_detector (Air box, 26.40 → 31.50 m)  embedded, sits in the cavity
```

The envelope values in `muon_shield.toml` are sized to contain the magnets
(3520 × 2640 mm, 4.54–32.08 m); the container is placed in the world at its Z
centre (18.31 m) by `SHiPGeometryBuilder`. The SND cavity is a real Boolean
subtraction (like FairShip's `SetSNDSpace`): the reservation box is subtracted
from whichever magnets it intersects — so which magnets get carved follows from
the box position, not a hard-coded list. See the **Reserved space** section
below.

## Configuration (`muon_shield.toml`)

| Key                  | Fallback | Meaning                                        |
|----------------------|----------|------------------------------------------------|
| `block_material`     | "Iron"   | Absorber material (must exist in SHiPMaterials) |
| `envelope_half_x_mm` | 1760     | Container half-X (mm)                           |
| `envelope_half_y_mm` | 1320     | Container half-Y (mm)                           |
| `envelope_z_start_m` | 4.54     | Envelope start along Z (m, world coords)        |
| `envelope_z_end_m`   | 32.08    | Envelope end along Z (m, world coords)          |
| `[[block]]`          | —        | One table per iron block (see below)            |

The **Fallback** column is the parser default used only when a key is *omitted*.
The shipped `muon_shield.toml` sets the envelope explicitly (half-sizes
1760 × 1320 mm, z = 4.54–32.08 m — the values in the geometry tree above), sized
to contain the blocks. The **envelope** is the Air container and the
subsystem's world placement.

The parser rejects an inverted or zero envelope, a block with a non-positive
size or a taper that collapses its downstream face, a block whose upstream face
lies outside the envelope in Z, a block whose extent leaves the envelope, and
two blocks that intersect. The last two are computed from the blocks' true
corners and use a separating-axis test on the oriented boxes, so rotation and
taper are accounted for: a rotated block that genuinely fits is accepted, and
rotated blocks that genuinely intersect are caught even when their axis-aligned
bounds are disjoint. Touching blocks remain legal.

### Blocks

Each `[[block]]` is a table with these fields (all lengths in mm, angles in
degrees, world/beamline coordinates):

```toml
[[block]]
start    = [0.0, 0.0, 4740.0]      # centre of the UPSTREAM (-z) face
size     = [3000.0, 2000.0, 21660.0]  # near-face full x, y, and z length
rotation = [0.0, 2.0, 0.0]         # optional: extrinsic X→Y→Z about `start`
taper    = [5.0, 0.0]              # optional: symmetric half-opening angles
```

- `start` is the centre of the block's upstream (−z) face; the block extends
  downstream by `size[2]` before rotation.
- `rotation` (default `[0,0,0]`) is applied about `start`, extrinsically about
  the world X then Y then Z axes.
- `taper` (default `[0,0]`) gives symmetric half-opening angles: the section
  widens (or, if negative, narrows) toward the downstream face, so the far
  half-x is `size[0]/2 + size[2]·tan(taper[0])` and similarly for Y. `[0,0]`
  yields a `GeoBox`; otherwise a `GeoTrd`.

### Reserved space (SND cavity)

The shield iron is carved by Boolean subtraction (A − B) so external detectors
can define their space independently. `MuonShieldFactory::reserveSpace(centre,
size, rotation)` registers a box (world coords, mm); at build time every magnet
the box intersects has it subtracted (`GeoShapeSubtraction`), while other
magnets are untouched. The **neutrino detector** reservation is declared in
`NeutrinoDetector/SD.toml` and wired in by `SHiPGeometryBuilder`, which reads the
envelope, calls `reserveSpace(...)`, then `embedDaughter(...)` to place the SND
in the cavity. Because the carve follows from the box position, the muon shield
never names which magnets hold the SND — moving the box in `SD.toml` moves the
cavity.

The cavity is carved at the declared `size` grown by `clearance_mm` on every
face (default 5 mm). Carving at exactly `size` would leave the detector's faces
coincident with the Boolean cut surfaces, which Geant4 navigation handles
poorly.

### Embedded daughters

Other subsystems can be nested inside the shield container while remaining
independent subsystems. `MuonShieldFactory::embedDaughter(vol, worldCentre,
rotation, name)` registers a pre-built volume, placed at build time at that
centre and orientation inside the container. The SND keeps its own factory,
config, and `/SHiP/neutrino_detector` naming — only its position in the volume
tree changes (it becomes a daughter of `/SHiP/muon_shield` rather than a direct
child of the world), sitting in the cavity carved by its `reserveSpace`
reservation.

`embedDaughter` and `reserveSpace` take the same centre and rotation, so a
caller reading one envelope passes it to both and the cavity cannot drift away
from the detector. `build()` enforces the rest: it takes the daughter's own
bounding box, places it, and requires the result to fit inside the container
*and* inside one reserved cavity. A daughter that outgrows its reservation — or
has none — is a build-time error rather than a silent overlap with solid iron.

## Materials

| Material | Density     | Usage             |
|----------|-------------|-------------------|
| Air      | 1.29 mg/cm³ | Container volume  |
| Iron     | 7.87 g/cm³  | Shield blocks     |

## Status

- [x] Explicit block list driven by `muon_shield.toml` (position, rotation, taper)
- [x] Default layout is the FairShip TRY_2026 magnets, as solid blocks
- [x] SND cavity carved by Boolean subtraction from `SD.toml`
- [ ] Magnetic field (handled elsewhere; blocks are passive iron)
- [ ] Aperture and yoke shape (each magnet is currently one solid block)

## TODO

- Model the aperture and yoke of each magnet instead of the solid outer
  envelope, once the engineering layout is available from the subsystem
  coordinator. The block list can express this today by arranging several
  blocks around the gap; it is the numbers that are missing.
