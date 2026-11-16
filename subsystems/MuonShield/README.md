# MuonShield

Active muon shield magnets for background suppression.

## Description

The MuonShield subsystem holds magnets M1-M7 of the FairShip TRY_2026 shield
(integration layout 2026-0.1). Each magnet is one solid iron box of the
magnet's outer size, as FairShip builds them: the aperture, yoke shape and
field are not modelled (the field comes from the field map, and #125 tracks
the real apertures). The hadron stopper (FairShip row 0) belongs to the Target
subsystem.

The scattering and neutrino detector sits in the end of M5 and the whole of
M6, as in FairShip. Those two magnets have the SND box (800 x 800 mm,
z = 22.89-27.99 m) cut out of their iron; Layout.h checks that the SND
container fills that hole.

## Geometry Tree

Positions are along the beam in the world frame (half-sizes x, y in mm).

```
/SHiP/muon_shield (Air, 3420 x 2540 x 27590 mm, centred at z = 18 385 mm)
 |- magn_1 (Iron box)               z =  4.59 -  7.59 m   1360 x  800
 |- magn_2 (Iron box)               z =  7.89 - 12.39 m   1490 x  890
 |- magn_3 (Iron box)               z = 12.55 - 17.05 m   1552 x  850
 |- magn_4 (Iron box)               z = 17.20 - 21.70 m   1592 x  600
 |- magn_5 (Iron box - SND hole)    z = 21.86 - 24.48 m   1340 x  520
 |- magn_6 (Iron box - SND hole)    z = 24.63 - 27.99 m   1500 x  630
 \- magn_7 (Iron box)               z = 28.18 - 32.18 m   1710 x 1270
```

The table is `MuonShieldFactory::k_magnets`. Its half-sizes are FairShip's
`(dX, dY, dZ)` from `shield_db["TRY_2026"]` in `python/geometry_config.py`,
and each magnet starts its `Zgap` after the previous one ends.

## Materials

| Material | Density   | Usage            |
|----------|-----------|------------------|
| Air      | 1.29 mg/cm³ | Container volumes |
| Iron     | 7.87 g/cm³  | Magnet pieces     |

## Status

- [x] Solid-block magnets M1-M7 (FairShip TRY_2026)
- [x] Hole for the SND in M5 and M6
- [ ] Real magnet apertures and yokes (#125)
- [ ] Concrete floor blocks under the magnets (FairShip `floor_blocks`)
