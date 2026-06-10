# NeutrinoDetector

Scattering and Neutrino Detector (SND).

## Description

The SND measures tau-neutrino interactions in the muon-shield region. It is
built as a single air container holding three sub-detectors in sequence along
the beam (upstream → downstream):

- **Veto** — an upstream PVT scintillator-bar station: three planes of seven
  bars (420 × 60 × 10 mm). Planes 0 and 1 run horizontally and are staggered by
  ±9 mm in Y; plane 2 runs vertically. Total depth 40 mm.
- **NuTarget** — a silicon/tungsten sampling target: 120 layers, each a 3.5 mm
  tungsten absorber plate followed by an X and a Y silicon tracking plane
  (0.3 mm), on a 400 × 400 mm plate at 11.3 mm pitch (total 1356 mm).
- **HCAL** — a hadronic calorimeter: three transverse sections (40, 50, 60 cm),
  14 layers each. Every layer is a 50 mm iron absorber, an X and a Y
  scintillating-fibre plane, and a polystyrene tile grid (50 × 50 × 10 mm tiles).

Like the calorimeter and straw tracker, all elements are `GeoPhysVol` with
hierarchical `/SHiP/neutrino_detector/...` names that reuse shared `GeoLogVol`s;
sensitive-detector assignment is done downstream by name.

## Approximation: scintillating-fibre planes

In the standalone SND simulation each fibre plane is two staggered sub-layers of
0.25 mm-diameter polystyrene fibres. Placing those individually would add on the
order of 300 000 volumes — far beyond the scale of any other subsystem (Trackers
9 600 straws, UpstreamTagger 16 300 tiles). Since tightly-packed 0.25 mm fibres
are essentially solid polystyrene, each 0.5 mm fibre plane is modelled here as a
single homogeneous polystyrene slab of the same thickness, with the X/Y plane
names preserved for readout. Fibre-level segmentation is a future refinement.

## Geometry tree

```text
/SHiP/neutrino_detector (Air, 800 × 800 × 5100 mm box)
 ├─ veto/P{0..2}_B{0..6}        21 PVT bars (3 planes × 7)
 ├─ target/L{0..119}/{W,Si_X,Si_Y}   120 W + 120 Si-X + 120 Si-Y
 └─ hcal/S{0..2}_L{0..13}/{Fe,FibreX,FibreY,tile_{c}_{r}}
        iron + 2 fibre slabs + tile grid per layer
        (8×8 / 10×10 / 12×12 tiles in sections 0 / 1 / 2)
```

Container child count: 21 + 360 + 4438 = 4819.

Position in world: z = 28 950 mm (centre of the 26.40–31.50 m WARM envelope).
The SND sits within the downstream end of the muon-shield region, so its
container deliberately overlaps the muon-shield envelope (declared in
`tests/test_consistency.cpp`, analogous to the trackers/magnet overlap).

## Materials

| Material    | Density     | Usage                              |
|-------------|-------------|------------------------------------|
| Tungsten    | 19.3 g/cm³  | Target absorber plates             |
| Silicon     | 2.33 g/cm³  | Target X/Y tracking planes         |
| Iron        | 7.87 g/cm³  | HCAL absorber plates               |
| Polystyrene | 1.05 g/cm³  | HCAL fibre planes and tiles        |
| PVT         | 1.032 g/cm³ | Veto bars                          |
| Air         | 1.29 mg/cm³ | Container                          |

All are taken from the central `SHiPMaterials` catalogue. Only the pure Silicon
material was new (added alongside this subsystem); the rest already existed
(Tungsten/Iron with the target/calorimeter, PVT/Polystyrene with the timing
detector and UBT).

## Tests

`test_neutrinodetector.cpp`:

- `NeutrinoDetectorWithinEnvelope` — the container is a `GeoBox` within the SND
  envelope limits (half-width/height ≤ 400 mm, half-length ≤ 2550 mm).
- `NeutrinoDetectorChildCount` — the container holds 4819 children.

## Status

- [x] C++ implementation (veto + target + HCAL)
- [x] Homogenised fibre planes
- [ ] Individual fibre segmentation (if/when needed)
- [ ] HYBRID muon-shield placement variant (currently WARM)
- [ ] Per-channel sensitive-detector readout
- [ ] Verification against the standalone SND reference

## TODO

- Confirm the WARM vs HYBRID placement and z-position with the integration team.
- Reconcile the target plate size (400 mm here, 440 mm in the SND design note).
- Replace homogenised fibre planes with segmented fibres if fibre-level
  resolution is required.
