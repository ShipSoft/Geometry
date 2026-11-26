# Calorimeter

Sampling calorimeter (ECAL + HCAL).

## Description

The Calorimeter subsystem builds a layered ECAL + HCAL stack. Each
calorimeter module is a tile in an `nx × ny` grid; each tile is built
from a sequence of layer types that interleave absorber plates,
scintillator bars and HPL fibre layers.

All parameters — layer sequences, thicknesses, module tiling — are
`constexpr` constants in `CalorimeterConstants.h`
(namespace `SHiP::geometry::calo`).

## Geometry tree

```
/SHiP/calorimeter (Air, 6000 × 7000 × 2900 mm — the fixed subsystem envelope)
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

## HPL fibre layout

Each HPL layer is a solid aluminium casing (`kPlateXY` × `kPlateXY` ×
`kHplThickness`) holding three rows of fibres (`kFiberDiameter` cladding,
`kFiberCoreDiameter` core). The rows are hexagonally close-packed and
centred in the casing:

```
  ┌──────────────────────────────────┐
  │    ( )( )( )( )( )( )( )         │  sublayer 2: n_mid − 1 fibres, z = +r√3
  │   ( )( )( )( )( )( )( )( )       │  sublayer 1: n_mid fibres,     z = 0
  │    ( )( )( )( )( )( )( )         │  sublayer 0: n_mid − 1 fibres, z = −r√3
  └──────────────────────────────────┘
```

- Fibres in a row touch (pitch = one fibre diameter), and every row is
  centred on the casing axis, so the outer rows sit half a pitch off the
  middle row and each middle fibre nests between two outer ones.
- `n_mid = floor(kPlateXY / kFiberDiameter)` = 1800, so a layer holds
  1799 + 1800 + 1799 = 5398 fibres (`kFibresPerHpl`).
- Rows are `r√3` ≈ 1.039 mm apart in z, so neighbouring fibres in adjacent
  rows touch. The stack is `2r(1 + √3)` ≈ 3.28 mm tall and centred on the
  casing midplane. All fibre centres lie within the 2.4 mm readout SiPM
  (|z| ≤ 1.2 mm), but the outer-row fibres reach |z| ≈ 1.64 mm, so about
  30 % of each outer-row core lies outside the SiPM's footprint.

The centres come from `calo::fibresPerSublayer()` and
`calo::fibreCentre(sublayer, index)` in `CalorimeterConstants.h`. The builder
places fibres with them, and they are public so that the calorimeter digitiser
can use the same functions rather than a copy that could drift apart.
`buildLayer` checks its inputs before it adds anything to the mother volume,
and throws if the casing is thinner than the close-packed stack or narrower
than two fibre diameters (the outer rows would be empty).

## Materials

| Material   | Density       | Composition         | Notes                       |
|------------|---------------|---------------------|-----------------------------|
| Air        | 1.29 mg/cm³   | already in catalog  | container & layer envelopes |
| Lead       | 11.34 g/cm³   | Pb                  | ECAL absorber plates        |
| Iron       | 7.87 g/cm³    | Fe                  | HCAL absorber plates        |
| PVT        | 1.032 g/cm³   | C₉H₁₀, mass-fraction-normalised | bar scintillator |
| Polystyrene| 1.05 g/cm³    | C₈H₈, mass-fraction-normalised  | HPL fibre core   |

Lead, PVT and Polystyrene are added by this subsystem; Air and Iron
were already present in the central `Materials` catalog.

## Validation

The parameters are validated at compile time by `static_assert`s in
`CalorimeterConstants.h`:

- `kModuleNX` and `kModuleNY` must be strictly positive.
- The total Z extent of the ECAL sequence + `kGapEcalHcal` + the HCAL
  sequence (`kTotalStackZ`, 2600 mm) must fit inside the container's full
  Z extent (`2 × kContainerHalfZ`, 2900 mm).
- The tiled modules' half-footprint in X and Y — centre to the outer edge of
  the outermost module — must fit inside the container's X/Y half-extents
  (`kContainerHalfX` / `kContainerHalfY`).
- `kFiberCoreDiameter` cannot exceed `kFiberDiameter`.
- `kPlateXY` must be a whole number of bars at both bar pitches.
- The close-packed HPL fibre stack must fit in `kHplThickness`, the middle
  fibre row must fit in `kPlateXY`, and the outer rows must hold one fibre
  fewer than the middle row.

A violation fails the build with a descriptive message.

## Tests

`test_calorimeter.cpp` exercises:

- `CalorimeterBuilds` — the factory returns a non-null `GeoPhysVol` of
  the expected envelope size.
- `CalorimeterHasChildren` — the container holds exactly
  `kModuleNX × kModuleNY × kVolsPerModule` children (588), so a dropped or
  duplicated layer placement cannot pass unnoticed.
- `TotalStackZMatchesReference` — `calo::kTotalStackZ` matches the
  hand-computed reference, guarding the layer-sequence transcription.
- `HplFibreCountsPerSublayer` — 1799 / 1800 / 1799 fibres per sublayer.
- `HplFibresCloseTouchWithoutOverlap` — every pair of neighbouring fibres,
  within a row and between rows, is exactly one diameter apart.
- `HplFibresInsideCasing` — the outermost fibres of each row stay inside
  the casing.
- `HplBuilderPlacesSharedCentres` — `buildLayer` places every fibre at
  `calo::fibreCentre()`.
- `HplBuilderRejectsThinCasing` — a casing thinner than the stack throws
  and leaves the mother volume empty.
- `HplBuilderRejectsNarrowCasing` — a casing narrower than two fibre
  diameters throws and leaves the mother volume empty.
- `HplFibreCountToleratesRounding` — a casing-to-pitch ratio that rounds
  just below a whole number does not lose a fibre.

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
