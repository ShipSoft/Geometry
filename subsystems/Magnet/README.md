# Magnet

Spectrometer dipole magnet.

## Description

The Magnet subsystem implements the SHiP spectrometer magnet. It consists of an iron yoke (box with rectangular cutout) with 4 aluminium coils and 4 vertical aluminium connectors. The coils are currently simplified as boxes; the GDML reference uses half-tubes.

## Geometry Tree

```
SHiPMagnet (Air, 6500×8600×5000 mm)
 ├─ magyoke (Iron, outer 6000×8600×2800 minus inner 4400×7000×2820)
 ├─ MCoil1 (Aluminium, 1600×800×3320 mm)  at (+2200, +3250, 0)
 ├─ MCoil2 (Aluminium)                     at (-2200, +3250, 0)
 ├─ MCoil3 (Aluminium)                     at (+2200, -3250, 0)
 ├─ MCoil4 (Aluminium)                     at (-2200, -3250, 0)
 ├─ CV_1 (Aluminium, 800×6500×250 mm)      at (+2600, 0, -1525)
 ├─ CV_2 (Aluminium)                        at (-2600, 0, -1525)
 ├─ CV_3 (Aluminium)                        at (+2600, 0, +1525)
 └─ CV_4 (Aluminium)                        at (-2600, 0, +1525)
```

Position in world: z = 89570 mm.

## Materials

| Material   | Density    | Usage               |
|------------|------------|---------------------|
| Air        | 1.29 mg/cm³ | Container volume    |
| Iron       | 7.87 g/cm³  | Yoke                |
| Aluminium  | 2.70 g/cm³  | Coils & connectors  |

## Status

- [x] C++ implementation (box approximation for coils)
- [ ] Replace box coils with tube geometry
- [ ] Verification against GDML

## TODO

- Replace box coils with GeoTubs half-tube geometry (GDML uses rmin=10 mm, rmax=800 mm)
- Verify yoke dimensions and coil positions against GDML reference
