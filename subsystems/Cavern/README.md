# Cavern

The experimental hall (ECN3) containing the SHiP detector.

## Description

The Cavern subsystem defines the world volume and the surrounding rock mass with excavated cavities for the muon shield tunnel and the main experiment hall.

## Geometry

### Coordinate System

- **Origin**: Target front face (first tungsten disk)
- **Z-axis**: Along beam direction (longitudinal, positive downstream)
- **Y-axis**: Vertical (against gravity)
- **X-axis**: Horizontal, perpendicular to beam (completes right-handed system)
- **Beam axis height**: 1.703 m above the TCC8 floor, 3.364 m above the ECN3 floor

### World Volume (cave)

- **Shape**: Box
- **Dimensions**: 400 m × 400 m × 400 m (half-sizes: 200 m each)
- **Material**: Air

### Cavern Rock

- **Shape**: Box with subtracted cavities
- **Base dimensions**: 40 m × 40 m × 280 m
- **Material**: Concrete
- **Position**: centred on the modelled tunnel and hall (z = −13.225 m)

### Layout reference

The cavities follow the integration layout SPSXXSHIP0002 version 2026-0.1
(EDMS 3287817 v1.1). Their cross-sections and the end wall are the inner
concrete surfaces of the civil-engineering solid in the CAD model
ST1967028_01 behind that drawing, sectioned across the beam.

- The TCC8 floor is 1.703 m below the beam and ends at z = 21.37 m.
- A 0.82 m stretch at the TCC8 width follows, with its floor 0.80 m lower.
- The ECN3 hall starts at z = 22.19 m, with its floor 3.364 m below the beam.
  Its end wall is at z = 122.18 m.
- The tunnel and hall walls are not centred on the beam. The values below are
  taken at mid-hall, because the walls drift by 0.35 mrad relative to the beam
  axis (34 mm over the hall).

Simplifications:

- The arched part of the TCC8 roof upstream of z = 6.2 m is modelled as the
  flat 5.761 m ceiling of the rest of the tunnel.
- ECN3 is cut off at the arch springline (y = 8.636 m). The crane ledges,
  which widen the hall by 0.5 m on each side above y = 5.84 m, and the arch
  itself (crown at y = 11.586 m) are not modelled.
- The CAD model has neither pit. The yoke pit and the target pit are carried
  over from the FairShip cavern (ShipCave): the yoke pit is centred on the
  spectrometer magnet (z = 89.72 m).
- Galleries, niches, doorways and shafts are left out.

### Subtracted Cavities (global frame)

| Cavity | Dimensions (full) | x range | y range | z range |
|--------|-------------------|---------|---------|---------|
| TCC8 tunnel (muon shield cavern) | 10.0 m × 7.464 m × 170 m | −3.587 … +6.413 m | −1.703 … +5.761 m | −148.63 … +21.37 m |
| Stair step | 10.0 m × 8.264 m × 0.82 m | −3.587 … +6.413 m | −2.503 … +5.761 m | 21.37 … 22.19 m |
| ECN3 hall (experiment cavern) | 16.0 m × 12.0 m × 99.99 m | −4.566 … +11.434 m | −3.364 … +8.636 m | 22.19 … 122.18 m |
| Yoke pit | 8.4 m × 1 m × 9 m | ±4.2 m | −4.364 … −3.364 m | 85.22 … 94.22 m |
| Target pit | 4 m × 1 m × 4 m | ±2 m | −2.703 … −1.703 m | −0.54 … +3.46 m |

## Materials

| Material | Density | Composition |
|----------|---------|-------------|
| Air | 1.29 mg/cm³ | 79% N, 21% O (by mass) |
| Concrete | 2.3 g/cm³ | 52% O, 33% Si, 15% Ca (simplified) |

## Status

- [x] C++ implementation
- [x] Cavity positions and cross-sections checked against the integration
      layout 2026-0.1 and its CAD model
- [ ] Material properties review

## TODO

- Confirm the pits with the civil-engineering drawings; the CAD model has none
- Review the rock material: concrete simplified to O/Si/Ca, while molasse
  would be the physical choice for muon transport through the rock
