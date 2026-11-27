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
 └─ for each (mx, my) in [0..nx-1] × [0..ny-1], one layer placement per layer:
      ├─ lead: Air envelope       └─ lead plate
      ├─ iron plate
      ├─ bar layer: Air envelope  └─ bars           (shared, one per bar type)
      └─ HPL: aluminium casing    └─ fibre cladding └─ core  (shared, one per orientation)
```

Every layer placement is named
`/SHiP/calorimeter/{ecal|hcal}/gl{n}…_MX{x}Y{y}` and carries the copy
number `calo::layerCopyNumber(module, layer)` =
`module * 1000 + layer`, unique across the calorimeter. Here
`module = calo::moduleIndex(x, y)` = `y * kModuleNX + x`, and `layer`
counts the module's layer placements from 0. `calo::moduleOfLayerCopyNumber()`
and `calo::layerOfLayerCopyNumber()` decode it.

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
`makeLayer` checks its inputs before it creates any volume, and throws if the
casing is thinner than the close-packed stack, narrower than two fibre
diameters (the outer rows would be empty), or so wide that a row holds
10,000 fibres or more (the copy numbers of neighbouring sublayers would
collide).

All HPL layers of one orientation are identical, so the factory builds one
filled casing per orientation with `calo_fibre_hp::makeLayer()` and places
it directly in all 48 slots (8 per module, 6 modules). GeoModel
and Geant4 then hold 2 × 5398 fibres instead of 48 × 5398, which shrinks the
geometry database from 105 MB to 20 MB. Geant4 still tracks through every
fibre in every layer.

Because the casing is shared, a fibre's name and copy number identify it
within its casing only: `HPL_{V|H}_S{sublayer}_F{index}_Clad`,
with copy number `calo::fibreCopyNumber(sublayer, index)` =
`sublayer * 10000 + index`. The layer and module come from the casing's
placement, `…/gl{n}_sl{m}_hpl_{y|x}_MX{x}Y{y}`, one level above the
cladding.

## Bar layers

Every scintillator bar layer of one type (`WidePVT_H`, `WidePVT_V`,
`ThinPS_H`, `ThinPS_V`) is identical, in the ECAL and the HCAL alike. The
factory builds each type once as an air envelope filled with its bars
(`calo_bar::makeLayer()`), and places that envelope in every slot of that
type across the six modules (270 bar-layer slots in all). GeoModel and Geant4 then hold 504 bars
(2 × 36 + 2 × 216) instead of 31 320.

A bar's name and copy number identify it within its layer: bar `i` is named
`{type}_B{i}` (for example `thin_ps_h_B17`) with copy number `i`. The layer
and module come from the envelope's placement,
`/SHiP/calorimeter/{ecal|hcal}/gl{n}_sl{m}_{type}_MX{x}Y{y}`, one level
above the bar. The bar logical volumes are
`/SHiP/calorimeter/{type}` and the envelopes `/SHiP/calorimeter/{type}_env`.

## Identifying hits

Counting up the Geant4 touchable history from the step point:

| Depth | Fibre step                                  | Bar step                                |
|-------|---------------------------------------------|-----------------------------------------|
| 0     | `HPL_Core`, no copy number                  | bar, copy number = bar index            |
| 1     | cladding, `fibreCopyNumber(sublayer, index)`; orientation (`V`/`H`) in the name | layer envelope, `layerCopyNumber()` |
| 2     | casing, `layerCopyNumber()`                 | `/SHiP/calorimeter` container           |
| 3     | `/SHiP/calorimeter` container               |                                         |

So for a fibre the layer is `GetCopyNumber(2)` and the fibre
`GetCopyNumber(1)`; for a bar the layer is `GetCopyNumber(1)` and the bar
`GetCopyNumber(0)`.

> [!WARNING]
> One Geant4 physical volume per bar type, and per fibre within a casing,
> is reused in every slot. A sensitive detector or digitiser that caches a
> lookup by `G4VPhysicalVolume*`, or keys it on the step volume's name,
> gets the first layer's layer and module for every layer, and nothing
> shows that it went wrong. Identify hits only through the touchable
> history (`GetCopyNumber(depth)` / `GetVolume(depth)`).

Sharing also means that all layers of one type are the same volume. A
per-layer misalignment or a dead channel in one layer cannot be added as
a transform or a change inside the shared volume; it means giving that
layer a volume of its own.

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
- A row holds fewer fibres than the `fibreCopyNumber()` sublayer stride,
  and a module fewer layer placements than the `layerCopyNumber()` module
  stride.

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
- `LayerCopyNumbersIdentifyModuleAndLayer` — every layer placement has a
  unique copy number that decodes to its module (checked against its
  position) and its layer.
- `HplBuilderPlacesSharedCentres` — `makeLayer` places every fibre at
  `calo::fibreCentre()`, with copy number `calo::fibreCopyNumber()`.
- `BarLayersSharedAcrossSlots` — every bar layer of one type places the
  same filled envelope, with each bar at its pitch and copy number = index.
- `HplCasingsSharedAcrossLayers` — the calorimeter places one casing per
  fibre orientation directly in every HPL slot.
- `HplBuilderRejectsThinCasing` — a casing thinner than the stack throws
  and leaves the mother volume empty.
- `HplBuilderRejectsNarrowCasing` — a casing narrower than two fibre
  diameters throws and leaves the mother volume empty.
- `HplBuilderRejectsCopyNumberOverflow` — a row of 10,000 fibres or more
  throws, since its copy numbers would run into the next sublayer's.
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
