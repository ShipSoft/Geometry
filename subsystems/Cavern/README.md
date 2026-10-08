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
- **Beam axis height**: 1.7 m above floor

### World Volume (cave)

- **Shape**: Box
- **Dimensions**: 400 m × 400 m × 400 m (half-sizes: 200 m each)
- **Material**: Air

### Cavern Rock

- **Shape**: Box with subtracted cavities
- **Base dimensions**: 40 m × 40 m × 280 m
- **Material**: Concrete
- **Position**: centred on the modelled tunnel and hall (z = −14.08 m)

### Layout reference

The cavities follow the integration layout SPSXXSHIP0002 version 2026-0.1
(EDMS 3287817 v1.1) and the CAD model ST1967028_01: the TCC8 floor ends at
z = 21.37 m, the 0.82 m stair step that follows is centred on the gap
between muon-shield magnets M4 and S5, the beam line is 1.70 m above the TCC8 floor and 3.36 m
above the ECN3 floor, the yoke pit is centred on the spectrometer magnet
(z = 89.72 m) and the ECN3 end wall is at z = 120.48 m. The transverse
offsets of the tunnel and the hall, the stair step and the pit sizes are
carried over from the FairShip cavern.

### Subtracted Cavities (global frame)

| Cavity | Dimensions (full) | x range | y range | z range |
|--------|-------------------|---------|---------|---------|
| TCC8 tunnel (muon shield cavern) | 9.99 m × 7.5 m × 170 m | −3.56 … +6.43 m | −1.70 … +5.80 m | −148.63 … +21.37 m |
| Stair step | 15.99 m × 11.2 m × 0.82 m | −4.56 … +11.43 m | −2.56 … +8.64 m | 21.37 … 22.19 m |
| ECN3 hall (experiment cavern) | 15.99 m × 12 m × 98.29 m | −4.56 … +11.43 m | −3.36 … +8.64 m | 22.19 … 120.48 m |
| Yoke pit | 8.4 m × 1 m × 9 m | ±4.2 m | −4.36 … −3.36 m | 85.22 … 94.22 m |
| Target pit | 4 m × 1 m × 4 m | ±2 m | −2.70 … −1.70 m | −0.54 … +3.46 m |

## Materials

| Material | Density | Composition |
|----------|---------|-------------|
| Air | 1.29 mg/cm³ | 79% N, 21% O (by mass) |
| Concrete | 2.3 g/cm³ | 52% O, 33% Si, 15% Ca (simplified) |

## Status

- [x] C++ implementation
- [x] Cavity positions checked against integration layout 2026-0.1
- [ ] Material properties review

## TODO

- Take the tunnel and hall cross-sections from the civil-engineering model
  instead of the FairShip values
- Review concrete composition (currently simplified to O/Si/Ca)
