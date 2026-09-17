# Calorimeter

Sampling calorimeter (ECAL + HCAL).

## Description

The Calorimeter subsystem builds a layered ECAL + HCAL stack. Each
calorimeter module is a tile in an `nx × ny` grid; each tile is built
from a sequence of layer types that interleave absorber plates,
scintillator bars and HPL fibre layers.

All parameters — layer sequences, thicknesses, module tiling — are
`constexpr` constants in `CalorimeterConstants.h`
(namespace `SHiPGeometry::Calo`).

## Geometry tree

```
/SHiP/calorimeter (Air, 6000 × 7000 × 2900 mm half-extents fixed by SHiPGeometry)
 └─ for each (mx, my) in [0..nx-1] × [0..ny-1]:
      └─ <layer_env_MXxYy>           (Air envelope, kPlateXY × kPlateXY × layer_thickness)
            └─ layer body            (Lead / Iron / WidePVT / ThinPS / HPL)
                  └─ ...             (fibres or bars, depending on layer type)
```

## Layer types

The `kEcalLayers` / `kHcalLayers` sequences in `CalorimeterConstants.h`
are arrays of the `LayerCode` enum:

| Enumerator   | Layer type                                          |
|--------------|-----------------------------------------------------|
| `WidePVT_H`  | WidePVT bar layer, bars along X (H orientation)     |
| `WidePVT_V`  | WidePVT bar layer, bars along Y (V orientation)     |
| `ThinPS_H`   | ThinPS bar layer, bars along X (H orientation)      |
| `ThinPS_V`   | ThinPS bar layer, bars along Y (V orientation)      |
| `FibreHPL_Y` | HPL fibre layer, fibres along Y                     |
| `FibreHPL_X` | HPL fibre layer, fibres along X                     |
| `Absorber`   | Absorber plate (Lead in ECAL section, Iron in HCAL) |
| `AirGap`     | Air gap (no volume placed, just advances Z cursor)  |

## Materials

| Material   | Density       | Composition         | Notes                       |
|------------|---------------|---------------------|-----------------------------|
| Air        | 1.29 mg/cm³   | already in catalog  | container & layer envelopes |
| Lead       | 11.34 g/cm³   | Pb                  | ECAL absorber plates        |
| Iron       | 7.87 g/cm³    | Fe                  | HCAL absorber plates        |
| PVT        | 1.032 g/cm³   | C₉H₁₀, mass-fraction-normalised | bar scintillator |
| Polystyrene| 1.05 g/cm³    | C₈H₈, mass-fraction-normalised  | HPL fibre core   |

Lead, PVT and Polystyrene are added by this subsystem; Air and Iron
were already present in the central `SHiPMaterials` catalog.

## Validation

The parameters are validated at compile time by `static_assert`s in
`CalorimeterConstants.h`:

- `kModuleNX` and `kModuleNY` must be strictly positive.
- The total Z extent of the ECAL sequence + `kGapEcalHcal` + the HCAL
  sequence must fit inside the container's Z half-extent.
- The X/Y footprint of the tiled modules must fit inside the
  container's X/Y half-extents.
- `kFiberCoreDiameter` cannot exceed `kFiberDiameter`.
- `kPlateXY` must be a whole number of bars at both bar pitches.

A violation fails the build with a descriptive message.

## Tests

`test_calorimeter.cpp` exercises:

- `CalorimeterBuilds` — the factory returns a non-null `GeoPhysVol` of
  the expected envelope size.
- `CalorimeterHasChildren` — the container has at least the expected
  number of children (`kModuleNX × kModuleNY × non-air-gap layers`).
- `TotalStackZMatchesReference` — `Calo::kTotalStackZ` matches the
  hand-computed reference, guarding the layer-sequence transcription.

## Status

- [x] ECAL + HCAL layer sequencer
- [x] Bar scintillator layers (`CaloBarLayer`)
- [x] HPL fibre layers (`CaloFibreHPLayer`)
- [x] Lead, PVT, Polystyrene materials
- [x] Module-array tiling
- [x] Compile-time envelope-fit validation
- [ ] Verification against a reference GDML

## TODO

- Cross-check fibre and bar dimensions against the SHiP TDR once the
  optimisation phase fixes the design.
- Reconsider whether the ECAL and HCAL sequences should be unified into
  a single sequence with an explicit ECAL/HCAL marker, once the HCAL
  detector choice (own design vs. LHCb HCAL) is finalised.
