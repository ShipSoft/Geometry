# Magnet

Spectrometer dipole magnet.

## Description

The Magnet subsystem implements the SHiP spectrometer magnet as an iron yoke (box with a rectangular cutout) and two vertical aluminium coil packs. The envelopes follow the integration CAD model ST1967028_01 (layout 2026-0.1, EDMS 3287817 v1.1): the yoke is 8.0 m wide, 8.5 m high and 3.51 m long, the coil packs are the reference coil of field map V21 (130 mm thick, 6.75 m high, 4.69 m long at x = ±2221 mm). The yoke aperture is not resolved in the CAD model and is kept at 7.0 m high; it is 4.6 m wide so that the coil packs sit inside it. The coils are simplified as boxes.

## Geometry tree

```
/SHiP/magnet (Air, 8000×8500×5000 mm)
 ├─ /SHiP/magnet/yoke   (Iron, 8000×8500×3511 mm outer, 4600×7000 mm cutout)
 ├─ /SHiP/magnet/coil_1 (Aluminium, 130×6750×4687 mm)  at (+2221, 0, 0)
 └─ /SHiP/magnet/coil_2 (Aluminium)                     at (-2221, 0, 0)
```

Position in world: z = 89720 mm (mid-plane of the 87.22–92.22 m slot in integration layout 2026-0.1, EDMS 3287817 v1.1).

## Materials

| Material   | Density    | Usage               |
|------------|------------|---------------------|
| Air        | 1.29 mg/cm³ | Container volume    |
| Iron       | 7.87 g/cm³  | Yoke                |
| Aluminium  | 2.70 g/cm³  | Coil packs          |

## Status

- [x] C++ implementation (box approximation for coils)
- [x] Yoke and coil envelopes checked against the integration CAD model
- [ ] Yoke aperture from the magnet design

## TODO

- Take the yoke aperture and the coil shape from the magnet design once available
