# DecayVolume

Decay vessel and surrounding veto system.

## Description

The DecayVolume subsystem implements the vacuum decay vessel where hidden-sector particles decay. Currently modelled as a rectangular aluminium vessel filled with pressurised helium. The GDML reference uses a conical frustum shape (pyramidal, tapering from upstream to downstream).

## Geometry Tree

```
DecayVolume (Aluminium, 2900×4750×50400 mm)
 └─ DecayVacuum (PressurisedHe90, 2860×4710×50360 mm)
```

Position in world: z = 58120 mm. Wall thickness: 20 mm.

## Materials

| Material        | Density       | Usage         |
|-----------------|---------------|---------------|
| Aluminium       | 2.70 g/cm³    | Vessel walls  |
| PressurisedHe90 | 2.12 mg/cm³   | Inner atmosphere |

## Status

- [x] C++ implementation (box approximation)
- [ ] Replace box with frustum shape
- [ ] Add surrounding veto tagger
- [ ] Verification against GDML

## TODO

- Replace rectangular vessel with conical frustum (GeoTrap or GeoSimplePolygonBrep) matching the GDML pyramidal shape
- Add surrounding veto tagger (liquid scintillator cells around the vessel)
- Verify vessel dimensions and wall thickness against GDML reference
