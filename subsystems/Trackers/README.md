# Trackers

Straw tube tracking stations.

## Description

The Trackers subsystem implements 4 tracking stations for the SHiP spectrometer.
Stations 1–2 sit upstream of the main spectrometer magnet, stations 3–4
downstream. Each station is a stack of 4 stereo views (layers); each view
contains a hollow aluminium frame and two staggered sub-layers of straw tubes.

Geometry parameters (straw radius, length, pitch, stereo angle, frame size)
were ported from the standalone `StrawTrackerBuilder` reference; the per-station
envelope and station Z positions are taken from `subsystem_envelopes.csv`.

## Geometry Tree

```
/SHiP/trackers (Air, 6000 × 6860 × 12000 mm)
 ├─ /SHiP/trackers/station_1 (Air, 6000 × 6860 × 1000 mm)   z = 84070 mm
 │    └─ Layer_j  [j = 0..3]   air box, rotated by ±2.3° about Z
 │         ├─ StrawFrame       hollow rectangle (Aluminium, GeoShapeSubtraction)
 │         │                   outer 4210 × 6230 mm, aperture 4010 × 6030 mm
 │         ├─ StrawSubLayer_0  air slab at z = -10.55 mm  (nominal)
 │         │    └─ Straw  [i = 0..299]   wall (Mylar) + gas (Ar/CO₂) tubes
 │         └─ StrawSubLayer_1  air slab at z = +10.55 mm  (Y-staggered)
 │              └─ Straw  [i = 0..299]
 ├─ /SHiP/trackers/station_2                                 z = 86070 mm
 ├─ /SHiP/trackers/station_3                                 z = 93070 mm
 └─ /SHiP/trackers/station_4                                 z = 95070 mm
```

Position in world: container centred at z = 89570 mm
(`(s_station1Z + s_station4Z) / 2`).

### Straw specification

| Parameter        | Value                                |
|------------------|--------------------------------------|
| Outer radius     | 10 mm (20 mm diameter)               |
| Length           | 4000 mm (along local X)              |
| Wall thickness   | 30 µm Mylar                          |
| Fill gas         | Ar/CO₂ 70/30 by mass                 |
| Pitch            | 20 mm                                |
| Straws/sub-layer | 300                                  |
| Sub-layers/view  | 2 (second is +½-pitch staggered in Y)|
| Stereo angles    | +2.3°, −2.3°, +2.3°, −2.3°          |

The straw axis is rotated +90° about Y so that the (locally Z-aligned)
`GeoTube` ends up pointing along the layer's X axis.

## Materials

| Material      | Density          | Composition                                | Usage                |
|---------------|------------------|--------------------------------------------|----------------------|
| Air           | 1.29 mg/cm³      | N 75.5%, O 23.1%, Ar 1.4%                  | Container, sub-layers |
| Aluminium     | 2.70 g/cm³       | Al                                         | View frames          |
| Mylar         | 1.39 g/cm³       | C₁₀H₈O₄ (PET, mass-fractioned)             | Straw walls          |
| ArCO2_70_30   | 1.56 × 10⁻³ g/cm³| Ar 70%, CO₂ 30% (by mass)                  | Straw gas (sensitive)|

`Mylar` and `ArCO2_70_30` are added to `SHiPMaterials::createMaterials()`
in this PR; the elements they require (C, H, O, Ar) were already present.

## Implementation notes

### Shared `GeoLogVol`s

The factory builds **one** `GeoLogVol` per repeated geometric element (straw
wall, straw gas, sub-layer envelope ×2, layer envelope, frame) and reuses it
across every placement. The 9600 straw placements therefore generate only two
straw `GeoLogVol`s instead of two per placement.

### Stereo rotation

Each layer envelope is built axis-aligned (straws along local X). The
`±2.3°` rotation about Z is applied at placement time inside
`buildStation()`, so the frame and both sub-layers rotate together with the
view — matching the physical reality where the frame holds the straws.

### Solid-wall straws

The straw wall is a **solid** Mylar `GeoTube` with the gas tube as its single
daughter. This avoids the mother/daughter overlap that hollow walls plus an
inner gas cylinder produce, which Geant4's overlap checker flags on every
straw. Physics-wise the result is identical: the gas material is used inside
the gas volume and Mylar everywhere else.

## Status

- [x] C++ implementation (full straw geometry)
- [x] Stereo views and material frames
- [x] Mylar walls and Ar/CO₂ gas materials
- [ ] Anode wires (currently omitted — see TODO)
- [ ] Verification against GDML
- [ ] Cross-check with FairShip's `strawtubesGeo`

## TODO

- Add a thin tungsten wire (~25 µm) on the gas tube axis if downstream
  simulation needs an explicit anode.
- Verify station positions and layer pitch against the GDML reference once
  it's available.
- Consider config-driving the geometry (analogous to `calo.cfg`) once a
  second tracker variant lands.
