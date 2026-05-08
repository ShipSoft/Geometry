# Calorimeter

Configurable sampling calorimeter (ECAL + HCAL).

## Description

The Calorimeter subsystem builds a layered ECAL + HCAL stack from a
configuration file. Each calorimeter module is a tile in an `nx × ny`
grid; each tile is built from a sequence of layer codes that interleave
absorber plates, scintillator bars and HPL fibre layers.

The geometry is fully data-driven: changing the layer sequence or the
module pitch in `calo.toml` reshapes the calorimeter without recompiling.

## Geometry tree

```
/SHiP/calorimeter (Air, 6000 × 7000 × 2900 mm half-extents fixed by SHiPGeometry)
 └─ for each (mx, my) in [0..nx-1] × [0..ny-1]:
      └─ <layer_env_MXxYy>           (Air envelope, plate_xy_mm × plate_xy_mm × layer_thickness)
            └─ layer body            (Lead / Iron / WidePVT / ThinPS / HPL)
                  └─ ...             (fibres or bars, depending on layer code)
```

Position in world: z = 96970 mm (configurable via `detector_offset_z_mm`).

## Layer codes

Used in the `layers` (ECAL) and `layers2` (HCAL) arrays in `calo.toml`:

| Code | Layer type                                        |
|------|---------------------------------------------------|
| 1    | WidePVT bar layer, bars along X (H orientation)   |
| 2    | WidePVT bar layer, bars along Y (V orientation)   |
| 3    | ThinPS bar layer, bars along X (H orientation)    |
| 4    | ThinPS bar layer, bars along Y (V orientation)    |
| 5    | HPL fibre layer, fibres along Y                   |
| 6    | HPL fibre layer, fibres along X                   |
| 7    | Absorber plate (Lead in ECAL section, Iron in HCAL) |
| 8    | Air gap (no volume placed, just advances Z cursor)  |

## Configuration

Geometry parameters live in `calo.toml`. The factory loads the file at
build time. Path resolution order:

1. The path passed to `CalorimeterFactory(materials, configPath)`
   (defaults to `"calo.toml"`).
2. `CALO_TOML_DEFAULT_PATH` — absolute path baked at compile time
   pointing at the source-tree copy.
3. `CALO_TOML_INSTALL_PATH` — absolute path baked at compile time
   pointing at the installed data directory.

The first existing path wins. This means tests, in-tree builds, and
installed deployments all find a `calo.toml` without explicit path
arguments.

### Example

```toml
# Module geometry
plate_xy_mm        = 2160
lead_thickness_mm  = 3
scint_thickness_mm = 10

module_nx = 2          # > 0 enforced at parse time
module_ny = 3

# ECAL layer sequence (layer codes from the table above)
layers = [
    7, 1, 7, 2, 7, 3, 7, 4, 7, 1, ...
]

# HCAL layer sequence
gap_ecal_hcal_mm  = 100
layers2           = [7, 1, 7, 2, 7, 1, 7, 2, 7, 1]
iron_thickness_mm = 170
```

Unknown top-level keys are reported on `stderr` at parse time so typos
or stale fields are surfaced rather than silently ignored.

The `layers` / `layers2` arrays accept both TOML native arrays
(`[1, 2, 3]`) and the legacy comma-separated string form (`"1,2,3"`)
for compatibility with previously-deployed configs.

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

At factory entry the configuration is validated against the fixed
container envelope:

- `module_nx` and `module_ny` must be strictly positive.
- The total Z extent of `layers` + `gap_ecal_hcal_mm` + `layers2` must
  fit inside the container's Z half-extent.
- The X/Y footprint of the tiled modules
  (`(module_nx, module_ny) × pitch`, with `pitch = module_pitch_x_mm` if
  set, otherwise `plate_xy_mm`) must fit inside the container's X/Y
  half-extents.
- `fiber_core_diameter_mm`, when set, cannot exceed
  `fiber_diameter_mm`.

Any violation throws `std::runtime_error` with a descriptive message
identifying the offending dimension.

## Tests

`test_calorimeter.cpp` exercises:

- `CalorimeterBuilds` — the factory returns a non-null `GeoPhysVol` of
  the expected envelope size.
- `CalorimeterHasChildren` — the container has at least the expected
  number of children (`module_nx × module_ny × non-air-gap layers`).
- `TotalStackZPositive` — `CalorimeterFactory::totalStackZ()` agrees
  with the layer-by-layer sum for a representative configuration.

## Status

- [x] Configuration parser (toml++)
- [x] ECAL + HCAL layer sequencer
- [x] Bar scintillator layers (`CaloBarLayer`)
- [x] HPL fibre layers (`CaloFibreHPLayer`)
- [x] Lead, PVT, Polystyrene materials
- [x] Module-array tiling with configurable pitch
- [x] Envelope-fit validation
- [x] Unknown-key surfacing
- [ ] Verification against a reference GDML

## TODO

- Cross-check fibre and bar dimensions against the SHiP TDR once the
  optimisation phase fixes the design.
- Reconsider whether `layers` / `layers2` should be unified into a
  single sequence with an explicit ECAL/HCAL marker, once the HCAL
  detector choice (own design vs. LHCb HCAL) is finalised.
