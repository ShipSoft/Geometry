# DecayVolume

Decay region and Surround Background Tagger (SBT).

## Description

The DecayVolume subsystem models the SHiP decay region together with the
**Surround Background Tagger (SBT)** that wraps it. The SBT is a 50 m
rectangular frustum (expanding from 2.0 × 3.0 m at the entrance to 4.0 × 6.0 m
at the exit) made of two parts:

- a **steel H-beam supporting structure** — vertical columns, segmented corner
  beams, top/bottom longitudinal beams and cross-beams, each H-beam modelled as
  three GeoBox pieces (two flanges + web); and
- **LAB liquid-scintillator sensor cells** in thin aluminium containers,
  tiling the four faces of the frustum.

The decay region itself is **helium**, but it is not sized by a fixed inset.
It is *derived* from where the SBT material actually sits (see
[`SBTEnvelope.h`](include/DecayVolume/SBTEnvelope.h)): the innermost surfaces —
the side and top/bottom scintillator containers, and the top/bottom longitudinal
beam inner flanges, which hang *below* the sensor plane into the decay region —
bound it directly. Because that inner surface is not linear in Z (the side
containers present a flat outer face over the first part of each sub-frustum to
clear the columns), the helium is built as a stack of GeoTraps, two per
sub-frustum, each strictly inscribed inside the material minus a small
clearance. This cannot overlap the structure or the sensors.

Everything is held in an **air container** (the SBT is part of the experiment's
support frame, not a sealed vessel). The previous monolithic aluminium box
vessel has been removed. The structure and sensor geometry is a faithful port
of the standalone SBT GeoModel builders; all pieces are placed flat (as direct
children of the container), which the clash-avoidance logic relies on.

## Geometry tree

```
/SHiP/decay_volume                         (Air box, 4400 × 6600 × 50400 mm)
 ├─ /SHiP/decay_volume/helium_0..19        (PressurisedHe90, 20 inscribed GeoTrap slabs)
 ├─ /SHiP/decay_volume/sbt/structure_*     (Iron, 312 GeoBox H-beam pieces)
 └─ /SHiP/decay_volume/sbt/sensors_*       (Al walls + LAB cells, 3380 GeoTrap)
```

Position in world: z = 58120 mm (centre), unchanged. The 50 m SBT is centred
on the container origin (entrance face at z = −25000 mm in the local frame).

### Structure (312 GeoBox)

| Group               | Count | Notes                                        |
|---------------------|------:|----------------------------------------------|
| Vertical columns    |    66 | 11 rows × 2 sides × 3 boxes                  |
| Corner beams        |   120 | 4 corners × 10 segments × 3 boxes (inclined) |
| Longitudinal beams  |    60 | 1/face (sub-frustum 0–4), 2/face (5–9); flanges only |
| Cross-beams         |    66 | 11 rows × 2 faces × 3 boxes                  |

### Helium (20 GeoTrap)

Two inscribed slabs per sub-frustum (10 sub-frusta): the slab boundaries fall at
each sub-frustum entrance and at the side-container flat-piece edge, so the slab
faces follow the sawtooth inner surface without cutting into it.

### Sensors (3380 GeoTrap)

130 containers (80 side + 50 top/bottom), each Z-split into 2 pieces at the
column front-flange edge for clash avoidance. Each piece is 7 aluminium walls +
6 LAB cells (13 GeoTraps): 130 × 2 × 13 = 3380. Cells are `GeoFullPhysVol`
(sensitive); walls are `GeoPhysVol`.

## Materials

| Material        | Density      | Usage                          |
|-----------------|--------------|--------------------------------|
| Air             | 1.205 mg/cm³ | Container                      |
| Iron            | 7.874 g/cm³  | H-beam supporting structure    |
| Aluminium       | 2.70 g/cm³   | Sensor container walls         |
| LAB             | 0.867 g/cm³  | Scintillator cells (new)       |
| PressurisedHe90 | 2.12 mg/cm³  | Central decay region           |

`LAB` (linear alkylbenzene, C 87.41% / H 12.59% by mass) was added to the
central `SHiPMaterials` catalogue for the SBT cells.

## Configuration

The SBT geometry is driven by [`sbt.toml`](sbt.toml), parsed into an
`SBTConfig` (toml++), following the same pattern as the calorimeter's
`calo.toml`. It exposes the frustum envelope, sub-frustum count, H-beam
cross-section, sensor container/cell parameters, helium clearance, the SBT
entrance Z, and all material names. Unknown keys are reported on stderr.

## Status

- [x] C++ implementation (SBT structure + sensors + derived helium)
- [x] Frustum shape (replaces the old box vessel approximation)
- [x] Surround Background Tagger integrated
- [x] sbt.toml configuration
- [ ] Verification against the standalone SBT / GDML reference

## Tests

`test_decayvolume` checks the container envelope, the 312 structure GeoBoxes,
and the 3400 GeoTrap children (20 helium slabs + 3380 sensors) for a total of
3712 direct children. Beyond the counts, it runs an exact separating-axis
overlap test between every helium slab and every SBT volume in the built tree,
asserting the helium neither protrudes into any material nor leaves a margin
beyond the configured clearance, and repeats that across 14 perturbed SBT
configurations so the guarantee survives re-parameterisation.
