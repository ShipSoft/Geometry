# MuonShield

Active muon shield magnets for background suppression.

## Description

The MuonShield subsystem implements 6 magnet stations that deflect muons away from the detector acceptance. The current implementation uses bounding-box approximations of the GDML arb8 (trapezoidal) shapes — each station contains 8 iron pieces (left/right middle magnets, left/right return yokes, and 4 top/bottom corner pieces).

## Geometry Tree

```
MuonShieldArea (Air, 3620×3400×29448 mm)
 ├─ MagnAbsorb_container (Air)  z = -13568.3 mm
 │   └─ 8 × Iron boxes (MiddleMagL/R, MagRetL/R, MagTop/BotLeft/Right)
 ├─ Magn1_container (Air)       z = -7263.3 mm
 │   └─ 8 × Iron boxes
 ├─ Magn2_container (Air)       z = +591.5 mm
 │   └─ 8 × Iron boxes
 ├─ Magn3_container (Air)       z = +5821.6 mm
 │   └─ 8 × Iron boxes
 ├─ Magn4_container (Air)       z = +9096.9 mm
 │   └─ 8 × Iron boxes
 └─ Magn5_container (Air)       z = +12385.1 mm
     └─ 8 × Iron boxes
```

Position in world: z = 16763.3 mm (centre of MuonShieldArea).

## Materials

| Material | Density   | Usage            |
|----------|-----------|------------------|
| Air      | 1.29 mg/cm³ | Container volumes |
| Iron     | 7.87 g/cm³  | Magnet pieces     |

## Status

- [x] C++ implementation (box approximations)
- [ ] Replace boxes with proper GeoTrap/arb8 shapes
- [ ] Verify field map integration points
- [ ] Verification against GDML

## TODO

- Replace bounding-box iron pieces with proper trapezoidal (GeoTrap or GeoGenericTrap) shapes matching the GDML arb8 vertices
- Add magnetic field regions (currently geometry only, no field)
- Verify station z-positions and piece dimensions against GDML reference
