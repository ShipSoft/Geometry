# NeutrinoDetector

Scattering and Neutrino Detector (SND).

## Description

The SND measures tau-neutrino interactions in the muon-shield region. It is
built as a single air container holding three sub-detectors in sequence along
the beam (upstream -> downstream):

- **Veto** - an upstream PVT scintillator-bar station: three planes of seven
  bars (420 x 60 x 10 mm). Planes 0 and 1 run horizontally and are staggered by
  +/-9 mm in Y; plane 2 runs vertically. Total depth 40 mm.
- **NuTarget** - a silicon/tungsten sampling target: 120 layers, each a 3.5 mm
  tungsten absorber plate followed by an X and a Y silicon tracking plane
  (0.3 mm), on a 400 x 400 mm plate at 11.3 mm pitch (total 1356 mm).
- **HCAL** - a hadronic calorimeter: three transverse sections (40, 50, 60 cm),
  14 layers each. Every layer is a 50 mm iron absorber, an X and a Y
  scintillating-fibre plane, and a polystyrene tile grid (50 x 50 x 10 mm tiles).

Like the calorimeter and straw tracker, all elements are `GeoPhysVol` with
hierarchical `/SHiP/neutrino_detector/...` names that reuse shared `GeoLogVol`s;
sensitive-detector assignment is done downstream by name.

## Scintillating-fibre planes

Each fibre plane is an air envelope filled with individual 0.25 mm-diameter
polystyrene fibres in two staggered sub-layers (sub-layer 1 offset by one radius
to close the inter-fibre gaps), matching the standalone SND simulation. Fibres
in the X plane run along X (measuring Y); fibres in the Y plane run along Y
(measuring X). This is ~336 000 fibres in total, so the SND is by far the
highest-volume subsystem; the per-plane air envelopes keep them spatially
bounded for navigation.

## Geometry tree

```text
/SHiP/neutrino_detector (Air, 640 x 640 x 4100 mm box)
 |- veto/P{0..2}_B{0..6}                21 PVT bars (3 planes x 7)
 |- target/L{0..119}/{W,Si_X,Si_Y}      120 W + 120 Si-X + 120 Si-Y
 \- hcal/S{0..2}_L{0..13}/
       |- Fe                            iron absorber
       |- FibreX, FibreY (air envelopes)
       |     \- 0.25 mm polystyrene fibres, 2 staggered sub-layers
       \- tile_{c}_{r}                  tile grid (8x8 / 10x10 / 12x12)
```

Direct container children: 21 + 360 + 4438 = 4819. Total volumes including the
individual fibres: 340819 (~336k fibres).

Position in world: z = 28 950 mm (centre of the 26.40-31.50 m WARM SND slot).
The SND is part of the muon shield, so its box container sits within the
downstream end of the muon-shield region; the resulting envelope overlap is
declared in `tests/test_consistency.cpp`, analogous to the trackers/magnet
overlap.

## Materials

| Material    | Density     | Usage                              |
|-------------|-------------|------------------------------------|
| Tungsten    | 19.3 g/cm3  | Target absorber plates             |
| Silicon     | 2.33 g/cm3  | Target X/Y tracking planes         |
| Iron        | 7.87 g/cm3  | HCAL absorber plates               |
| Polystyrene | 1.05 g/cm3  | HCAL fibres and tiles              |
| PVT         | 1.032 g/cm3 | Veto bars                          |
| Air         | 1.29 mg/cm3 | Container and fibre-plane envelopes|

All are taken from the central `SHiPMaterials` catalogue. Only the pure Silicon
material was new (added alongside this subsystem); the rest already existed.

## Tests

`test_neutrinodetector.cpp`:

- `NeutrinoDetectorIsBoxContainer` - the container is a `GeoBox` large enough to
  hold the contents.
- `NeutrinoDetectorChildCount` - the container has 4819 direct children.
- `NeutrinoDetectorFibreCount` - the recursive descendant count is 340819,
  guarding the individual-fibre placement.

## Status

- [x] C++ implementation (veto + target + HCAL)
- [x] Individual scintillating fibres
- [x] Box container (part of the muon shield)
- [ ] HYBRID muon-shield placement variant (currently WARM)
- [ ] Per-channel sensitive-detector readout
- [ ] Verification against the standalone SND reference

## TODO

- Confirm the WARM vs HYBRID placement and z-position with the integration team.
- Add per-channel sensitive-detector readout once the SD layer is integrated.
