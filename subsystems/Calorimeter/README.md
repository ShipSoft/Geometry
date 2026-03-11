# Calorimeter

Electromagnetic and hadronic calorimeter.

## Description

The Calorimeter subsystem implements the SHiP calorimeter system consisting of three components: ECAL front, ECAL back, and HCAL. Currently all three are empty air boxes at the correct positions and sizes. The full implementation requires Pb/scintillator sampling layers for the ECAL and Fe/scintillator sampling layers for the HCAL.

## Geometry Tree

```
CalorimeterContainer (Air, 6000×7000×2900 mm)
 ├─ ECAL_front (Air, 4500×7000×200 mm)   z = 96970 mm
 ├─ ECAL_back  (Air, 5500×7000×600 mm)   z = 98370 mm
 └─ HCAL       (Air, 6000×7000×1000 mm)  z = 99270 mm
```

Position in world: z = 98320 mm (centre of container).

## Materials

| Material | Density     | Usage           |
|----------|-------------|-----------------|
| Air      | 1.29 mg/cm³ | Container & placeholders |

## Status

- [x] C++ implementation (envelope only)
- [ ] Implement ECAL Pb/scintillator sampling layers
- [ ] Implement HCAL Fe/scintillator sampling layers
- [ ] Verification against GDML

## TODO

- Implement ECAL with Pb/scintillator sandwich structure
  - ECAL front: pre-shower section
  - ECAL back: main EM section with cell segmentation
- Implement HCAL with Fe/scintillator sandwich structure
- Add Lead material to SHiPMaterials
- Verify component dimensions and positions against GDML reference
