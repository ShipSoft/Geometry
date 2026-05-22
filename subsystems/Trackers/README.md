# Trackers

Straw tube tracking stations.

## Description

The Trackers subsystem implements 4 straw tracking stations for the SHiP
spectrometer. Each station envelope is filled with 4 stereo views; each
view is a material frame (FairShip-style hollow rectangle) enclosing a
staggered double sub-layer of straw tubes.

A straw is a Mylar-walled cylinder filled with an Ar/CO₂ gas mixture. The
four views per station are rotated about the beam axis by alternating
stereo angles so that crossing straws give a space point.

The spectrometer dipole between stations 2 and 3 is a separate subsystem
(see `subsystems/Magnet`) and is not described here.

## Geometry tree

```
/SHiP/trackers (Air, 3000 × 3430 × 6000 mm half-extents)
 ├─ /SHiP/trackers/station_<n> (Air, 3000 × 3430 × 500 mm)   n = 1..4
 │    └─ /SHiP/trackers/station_<n>/view_<v>/envelope (Air)   v = 0..3
 │       (rotated about Z by the view stereo angle)
 │         ├─ .../frame_body      (Aluminium, hollow rectangle = outer − aperture)
 │         ├─ .../sublayer_0_body (Air slab, 300 straws)
 │         └─ .../sublayer_1_body (Air slab, 300 straws, half-pitch staggered)
 │               └─ .../straw_<i>
 │                    └─ straw_wall_<uid> (Mylar tube)
 │                         └─ straw_gas_<uid> (ArCO2_70_30 tube)
 └─ /SHiP/trackers/tracker_magnet (Air, inert marker box)
```

Station Z positions (centres): 84070, 86070, 93070, 95070 mm.
Position in world: centred at z = 89570 mm (average of stations 1 and 4).
Stations 1-2 are upstream of the magnet, stations 3-4 downstream.

## Tracker magnet

`/SHiP/trackers/tracker_magnet` is an inert, air-filled marker volume for
the tracker magnet, centred at z = 86820 mm with a 460 mm Z extent.

It is **not** a physically-scaled dipole. The spectrometer dipole proper is
the separate `Magnet` subsystem (iron yoke + coils) occupying z = 87.07 to
92.07 m. The only space inside the trackers container that is clear of both
the straw stations and that yoke is the ~0.5 m gap between station 2 and the
yoke, so the marker is sized and placed to fit there without overlap.

The marker exists so the tracker magnet has a named placeholder in the
geometry; simulation/field code can locate it by its log-volume name. A
physically-scaled magnet, if required, belongs in the `Magnet` subsystem.

## Parameters

| Parameter | Value |
| --- | --- |
| Stations | 4 |
| Stereo views per station | 4 |
| Sub-layers per view | 2 (half-pitch staggered) |
| Straws per sub-layer | 300 |
| Straws total | 4 × 4 × 2 × 300 = 9600 |
| Straw outer diameter | 20 mm |
| Straw length | 4000 mm (along X) |
| Straw wall | 30 µm Mylar |
| Straw fill | Ar/CO₂ 70/30 by mass |
| View aperture | 4000 × 6000 mm (X × Y) |
| View stereo angles | +2.3°, −2.3°, +2.3°, −2.3° about Z |
| Frame bar width | 100 mm (X and Y) |
| Frame material | Aluminium |

## Materials

| Material | Density | Composition | Notes |
| --- | --- | --- | --- |
| Air | 1.29 mg/cm³ | already in catalog | container, station, view, sub-layer envelopes |
| Aluminium | 2.70 g/cm³ | already in catalog | view frames |
| Mylar | 1.39 g/cm³ | C₁₀H₈O₄, mass-fraction-normalised | straw walls |
| ArCO2_70_30 | 1.56 mg/cm³ | 70/30 Ar/CO₂ by mass | straw gas fill |

Mylar and ArCO2_70_30 are added by this subsystem to the central
`SHiPMaterials` catalog; Air and Aluminium were already present. All
elements required (C, H, O, Ar) were already in the element catalog.

## Tests

`test_trackers.cpp` exercises:

- `TrackersWithinEnvelope` — station 1 exists and its box stays within the
  CSV envelope limits (≤ 3000 × 3500 × 500 mm half-extents).
- `TrackersHasFourStations` — all 4 station volumes are present.
- `TrackersStationHasViews` — each station holds 4 stereo views.
- `TrackersViewHasFrameAndSubLayers` — a view holds a frame plus two
  sub-layers, and each sub-layer carries the full straw count.
- `TrackersHasTrackerMagnet` — the inert `tracker_magnet` marker exists and
  fits in the gap before the spectrometer-magnet yoke (no overlap).

## Status

- [x] C++ implementation (straw-level geometry)
- [x] 4 stations × 4 stereo views
- [x] Straw tubes (Mylar wall + Ar/CO₂ gas)
- [x] Staggered double sub-layers
- [x] FairShip-style view frames
- [x] Inert TrackerMagnet marker volume
- [x] Mylar and ArCO2_70_30 materials
- [ ] Verification against a reference GDML

## TODO

- Verify straw pitch, view spacing and station positions against the GDML
  reference once the tracker design is fixed.
- Add support frames and service volumes beyond the per-view frame.
