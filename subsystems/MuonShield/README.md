# MuonShield

Magnetised hadron stopper and active muon shield magnets for background
suppression.

## Description

The MuonShield subsystem holds the magnetised hadron stopper and magnets M1-M7
of the FairShip TRY_2026 shield (integration layout 2026-0.1). The stopper and
each magnet are one solid iron box of the magnet's outer size, as FairShip
builds them. The apertures and yoke shapes are not modelled (#125). The field
is not modelled either: it comes from the field map.

## Geometry Tree

Positions are along the beam in the world frame (half-sizes x, y in mm).

```
/SHiP/muon_shield (Air, 3420 x 3380 x 30040 mm, centred at z = 17 160 mm)
 |- magn_absorb (Iron box)          z =  2.14 -  4.44 m   1635 x 1690
 |- magn_1 (Iron box)               z =  4.59 -  7.59 m   1360 x  800
 |- magn_2 (Iron box)               z =  7.89 - 12.39 m   1490 x  890
 |- magn_3 (Iron box)               z = 12.55 - 17.05 m   1552 x  850
 |- magn_4 (Iron box)               z = 17.20 - 21.70 m   1592 x  600
 |- magn_5 (Iron box)               z = 21.86 - 24.48 m   1340 x  520
 |- magn_6 (Iron box)               z = 24.63 - 27.99 m   1500 x  630
 \- magn_7 (Iron box)               z = 28.18 - 32.18 m   1710 x 1270
```

The blocks are `MuonShieldFactory::k_hadronStopper` (`magn_absorb`, FairShip's
row 0) and `MuonShieldFactory::k_magnets`. Their half-sizes are FairShip's
`(dX, dY, dZ)` from `shield_db["TRY_2026"]` in `python/geometry_config.py`.
The stopper starts at 2.14 m, which is FairShip's `hadronAbsorber.z` and the
start of the EDMS envelope, and each magnet starts its `Zgap` after the
previous one ends. The container spans the blocks exactly and is derived from
them at compile time.

## Materials

| Material | Density   | Usage            |
|----------|-----------|------------------|
| Air      | 1.29 mg/cm³ | Container volume |
| Iron     | 7.87 g/cm³  | Stopper and magnets |

## Status

- [x] Solid-block hadron stopper and magnets M1-M7 (FairShip TRY_2026)
- [ ] Real magnet apertures and yokes (#125)
- [ ] Iron absorber around the hadron stopper (FairShip `AbsorberVol`)
