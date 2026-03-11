# Trackers

Straw tube tracking stations.

## Description

The Trackers subsystem implements 4 tracking stations for the SHiP spectrometer. Each station currently consists of an empty air box at the correct z-position. The full implementation requires straw tube modules with individual straws, stereo views, and support structures.

## Geometry Tree

```
TrackersContainer (Air, 6000×6860×6000 mm)
 ├─ TrackerStation_1 (Air, 6000×6860×1000 mm)  z = 84070 mm
 ├─ TrackerStation_2 (Air)                       z = 86070 mm
 ├─ TrackerStation_3 (Air)                       z = 93070 mm
 └─ TrackerStation_4 (Air)                       z = 95070 mm
```

Position in world: centred at z = 89570 mm (average of stations 1 and 4).
Stations 1-2 are upstream of the magnet, stations 3-4 downstream.

## Materials

| Material | Density     | Usage                |
|----------|-------------|----------------------|
| Air      | 1.29 mg/cm³ | Container & stations |

## Status

- [x] C++ implementation (envelope only)
- [ ] Implement straw tube geometry
- [ ] Add stereo views and support structures
- [ ] Verification against GDML

## TODO

- Implement straw tube modules within each station (major work)
  - Individual straw tubes (mylar + gas)
  - 4 views per station (Y, U, V, Y') with stereo angles
  - Support frames and service volumes
- Add straw tube gas material (Ar/CO2 mixture) to SHiPMaterials
- Add mylar material to SHiPMaterials
- Verify station positions against GDML reference
