# TimingDetector

Timing detector for time-of-flight measurements.

## Description

The TimingDetector subsystem implements 330 scintillator bars (3 columns x 110 rows) for precise timing measurements. It is the most detailed subsystem, built via GeoModelXML (`Gmx2Geo`) from an XML prototype file. Each bar is a `GeoFullPhysVol` registered as a sensitive volume through `SHiPTimingDetInterface`.

## Geometry Tree

```
Timing_Detector (Air, 5500×6500×500 mm)
 └─ 330 × scintillator bars (1400×60×10 mm each)
     built from GeoModelXML prototype
```

Position in world: z = 95902 mm.

## Materials

| Material     | Density      | Usage           |
|--------------|--------------|-----------------|
| Air          | 1.29 mg/cm³  | Container volume |
| Scintillator | 1.023 g/cm³  | Detector bars   |

## Status

- [x] C++ implementation (GeoModelXML)
- [x] Sensitive volume registration
- [ ] Validate bar positions against FairShip reference
- [ ] Verification against GDML

## TODO

- Validate bar positions and spacing against FairShip reference geometry
- Verify bar count (330 = 3 x 110) matches FairShip
- Review Z stagger pattern between columns
