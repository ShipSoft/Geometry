# TimingDetector

Timing detector for time-of-flight measurements.

## Description

The TimingDetector subsystem implements 330 scintillator bars (3 columns x 110 rows) for precise timing measurements. It is built directly in C++ (`TimingDetectorFactory`): the bars share a single `GeoLogVol` and are placed as `GeoPhysVol` with hierarchical `/SHiP/timing_detector/bar_<col>_<row>` names, so sensitive-detector assignment happens downstream by name pattern. Bar positions are computed analytically (see the factory).

## Geometry Tree

```
Timing_Detector (Air, 5500×6500×500 mm)
 └─ 330 × scintillator bars (1400×60×10 mm each)
     3 columns × 110 rows; z stagger = (row%2)·12 + (col%2)·90 mm
```

Position in world: z = 95902 mm.

## Materials

| Material     | Density      | Usage            |
|--------------|--------------|------------------|
| Air          | 1.29 mg/cm³  | Container volume |
| TimDetScint  | 1.023 g/cm³  | Detector bars    |

## Status

- [x] C++ implementation
- [x] Name-pattern sensitive-volume identification
- [ ] Validate bar positions against FairShip reference
- [ ] Verification against GDML

## TODO

- Validate bar positions and spacing against FairShip reference geometry
- Verify bar count (330 = 3 x 110) matches FairShip
- Review Z stagger pattern between columns
