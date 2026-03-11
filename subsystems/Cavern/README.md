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
- **Position**: z = 20.52 m from world origin

### Subtracted Cavities

| Cavity | Dimensions (full) | Position (x, y, z) |
|--------|-------------------|-------------------|
| Muon shield cavern | 9.99 m × 7.5 m × 170 m | (1.435, 2.05, -85) m |
| Experiment cavern | 15.99 m × 12 m × 99.18 m | (3.435, 2.64, 50.41) m |
| Stair step | 15.99 m × 11.2 m × 0.82 m | (3.435, 3.04, 0.41) m |
| Yoke pit | 8.4 m × 1 m × 9 m | (0, -3.86, 69.05) m |
| Target pit | 4 m × 1 m × 4 m | (0, -2.2, -21.03) m |

## Materials

| Material | Density | Composition |
|----------|---------|-------------|
| Air | 1.29 mg/cm³ | 79% N, 21% O (by mass) |
| Concrete | 2.3 g/cm³ | 52% O, 33% Si, 15% Ca (simplified) |

## Status

- [x] C++ implementation
- [ ] Verify cavity dimensions against GDML
- [ ] Material properties review

## TODO

- Verify cavity positions and dimensions against GDML reference
- Review concrete composition (currently simplified to O/Si/Ca)
