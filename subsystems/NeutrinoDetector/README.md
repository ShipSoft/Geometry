# NeutrinoDetector

Scattering and Neutrino Detector (SND).

## Description

The SND measures tau-neutrino interactions in the muon-shield region. It is
built as a single air container holding four sub-detectors in sequence along
the beam (upstream -> downstream):

- **Veto** - an upstream PVT scintillator-bar station: three planes of seven
  bars (420 x 60 x 10 mm). Planes 0 and 1 run horizontally and are staggered by
  +/-9 mm in Y; plane 2 runs vertically. Total depth 40 mm.
- **NuTarget (HGC)** - a silicon/tungsten sampling target: 120 layers, each a 3.5 mm
  tungsten absorber plate followed by an X and a Y silicon tracking plane
  (0.3 mm), on a 400 x 400 mm plate at 11.3 mm pitch.
  3 timing layers (18 mm each: a 10 mm silicon station and 8 mm of air) are inserted
  after layers L39, L79 and L119.
  The total depth of the target is 1410 mm.
- **MGC** - a medium-granularity calorimeter: 10 layers at 20 mm pitch on a 500 x 500 mm plate,
  each a tungsten absorber plate (19.25 mm) followed by a silicon tracking plane (0.65 mm).
- **HCAL (MTC)** - a hadronic calorimeter: three transverse sections (60, 60, 60 cm),
  14 layers each. Every layer is a 50 mm iron absorber, an X and a Y
  scintillating-fibre plane, and a polystyrene tile grid (50 x 50 x 29 mm tiles).
  The HCAL is magnetised: its field comes from the muon-shield field map, so a
  realistic simulation of it needs the field-map placement from the field_maps
  repository.

Like the calorimeter and straw tracker, all elements are `GeoPhysVol` with
hierarchical `/SHiP/neutrino_detector/...` names that reuse shared `GeoLogVol`s;
sensitive-detector assignment is done downstream by name.

## Scintillating-fibre planes

Each fibre plane is an air envelope filled with individual 0.25 mm-diameter
polystyrene fibres in two staggered sub-layers (sub-layer 1 offset by one radius
to close the inter-fibre gaps), matching the standalone SND simulation. Fibres
in the X plane run along X (measuring Y); fibres in the Y plane run along Y
(measuring X). This is ~403 200 fibres in total, so the SND is by far the
highest-volume subsystem; the per-plane air envelopes keep them spatially
bounded for navigation.

Within a section, every X plane is identical and so is every Y plane. Each
section's two planes are therefore built once as filled envelopes and placed
in all 14 of its layers. GeoModel's Geant4 conversion makes one logical
volume per filled envelope, so Geant4 holds 6 fibre planes (24 000 fibre
volumes) instead of 84 (336 000), while still tracking through every fibre of
every plane. In a standalone neutrino detector this cuts gmclash's peak RSS
from 167 MB to 66 MB, and geantinos along the beam take the same steps in
about a quarter of the time.

Because the planes are shared, the fibres inside a plane are named after the
section, `hcal/S{s}/FibreX` (or `FibreY`), with copy numbers 0 … 2n − 1
running across the two sub-layers. The layer comes from the envelope's
placement, `hcal/S{s}_L{l}/FibreX`, one level up the touchable history.

## Geometry tree

```text
/SHiP/neutrino_detector (Air, stepped union of 4 boxes, 602.5 x 602.5 x 5050 mm bounding box)
 |- veto/P{0..2}_B{0..6}                21 PVT bars (3 planes x 7)
 |- target/
       |- L{0..119}/{W,Si_X,Si_Y}      120 W + 120 Si-X + 120 Si-Y
       |- L{39,79,119}/Timing_Si       3 timing stations
 |- mgc/L{0..9}/{W,Si_Station}          10 W + 10 Si-Station
 |- hcal/S{0..2}_L{0..13}/
       |- Fe                            iron absorber
       |- FibreX, FibreY (air envelopes)
       |     \- 0.25 mm polystyrene fibres, 2 staggered sub-layers
       \- tile_{c}_{r}                  tile grid (12x12 / 12x12 / 12x12)
```

Direct container children: 21 + 363 + 20 + 6174 = 6578. Total volumes including the
individual fibres: 409778 (403200 fibres).

Position in world: z = 25 465 mm (22.94-27.99 m: the end of muon-shield
magnet M5 and the whole of M6, as in FairShip's TRY_2026 layout). This is
3.46 m upstream of the EDMS envelope (26.40-31.50 m), which follows the WARM
shield layout; `Layout.h` records it as a bounded known deviation.

The container is exactly as long as its content (5.05 m), laid out from its
downstream end, so the HCAL (24.63-27.99 m) fills M6 exactly. It is not a
single box but the union of four boxes, one per sub-detector, each 1 mm wider
than it (`NeutrinoDetectorFactory::k_sections`): veto 211 x 220 mm, target
201 x 201, MGC 251 x 251 and HCAL 301.25 x 301.25 (half-sizes). The gaps
between sub-detectors belong to the narrower box beside them. The muon shield
cuts the same boxes out of the M5 and M6 iron, so the iron comes up to each
sub-detector.

The SND is still inside the muon-shield container, so the two containers
overlap (#117, declared in `Layout.h`).

## Materials

| Material    | Density     | Usage                                |
|-------------|-------------|--------------------------------------|
| Tungsten    | 19.3 g/cm3  | Target absorber plates               |
| Silicon     | 2.33 g/cm3  | Target X/Y planes, timing, MGC pads  |
| Iron        | 7.87 g/cm3  | HCAL absorber plates                 |
| Polystyrene | 1.05 g/cm3  | HCAL fibres and tiles                |
| PVT         | 1.032 g/cm3 | Veto bars                            |
| Air         | 1.29 mg/cm3 | Container and fibre-plane envelopes  |

All are taken from the central `Materials` catalogue. Only the pure Silicon
material was new (added alongside this subsystem); the rest already existed.

## Tests

`test_neutrinodetector.cpp`:

- `NeutrinoDetectorChildCount` - the container has 6578 direct children.
- `FibrePlanesSharedWithinSection` - each section places one shared X plane
  and one shared Y plane in all its layers, with section-level fibre names
  and continuous copy numbers.

## Status

- [x] C++ implementation (veto + target + HCAL)
- [x] Individual scintillating fibres
- [x] Stepped container following the sub-detectors, in a cavity of M5 and M6
- [x] Placement in the TRY_2026 muon shield (M5 and M6)
- [ ] Per-channel sensitive-detector readout
- [ ] Verification against the standalone SND reference

## TODO

- Confirm the TRY_2026 placement with the integration team: the EDMS envelope still follows the WARM layout.
- Add per-channel sensitive-detector readout once the SD layer is integrated.
