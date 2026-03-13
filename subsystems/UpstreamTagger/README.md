# UpstreamTagger

Upstream Background Tagger (UBT) — segmented drift tube and scintillator tile detector.

## Description

The UpstreamTagger implements the UBT as a segmented 2×3 m² tracking plane placed at
z = 32720 mm, upstream of the decay vessel. It consists of:

- **Drift tubes** — mylar straw tubes (5 mm diameter, 15 µm wall, ArCO2 70/30 fill, 1.2 m long)
  arranged in double-staggered layers
- **Scintillator tiles** — 1×1×1 cm³ polystyrene tiles in two 40×40 blocks

## Geometry Tree

```
UBT_Envelope_LV  (Air, 2000×3000×16 mm)
├── UBT_Top_Left_LOG       (Air, outer top band, left half-plane)
│   ├── UBT_Top_Left_S0_T*_Wall  (Mylar, hollow tube annulus)
│   └── UBT_Top_Left_S0_T*_Gas   (ArCO2, solid cylinder) [sensitive]
│   ├── UBT_Top_Left_S1_T*_Wall
│   └── UBT_Top_Left_S1_T*_Gas   [sensitive]
├── UBT_Top_Right_LOG      (Air, outer top band, right half-plane)
│   └── ...
├── UBT_Bottom_Left_LOG    (Air, outer bottom band, left half-plane)
│   └── ...
├── UBT_Bottom_Right_LOG   (Air, outer bottom band, right half-plane)
│   └── ...
├── UBT_Central_LOG        (Air, central tube strip x=[-600,+600])
│   ├── UBT_Central_S*_T*_Wall
│   └── UBT_Central_S*_T*_Gas    [sensitive]
├── UBT_TileLeft_LOG       (Air, left tile block x=[-1000,-600])
│   └── UBT_TileLeft_T*_*        [sensitive]
└── UBT_TileRight_LOG      (Air, right tile block x=[+600,+1000])
    └── UBT_TileRight_T*_*       [sensitive]
```

## Detector Layout

```
  Y
  +1500 ┌──────────────────────────────────────────┐
        │  Top_Left tubes  +  Top_Right tubes       │  y = [+200, +1500] mm
  +200  ├───────────┬──────────────────┬────────────┤
        │ PS tiles  │  Central tubes   │  PS tiles  │  y = [-200, +200] mm
        │ (left)    │  x=[-600,+600]   │  (right)   │
  -200  ├───────────┴──────────────────┴────────────┤
        │  Bottom_Left tubes + Bottom_Right tubes   │  y = [-1500, -200] mm
  -1500 └──────────────────────────────────────────┘
        -1000 -600                  +600 +1000     → X (mm)
```

### Drift tubes

| Region | X extent (mm) | Y extent (mm) | Z centre (mm) |
|--------|--------------|---------------|---------------|
| Top_Left | [-1000, +200] | [+200, +1500] | -5 |
| Top_Right | [-200, +1000] | [+200, +1500] | +5 |
| Bottom_Left | [-1000, +200] | [-1500, -200] | -5 |
| Bottom_Right | [-200, +1000] | [-1500, -200] | +5 |
| Central | [-600, +600] | [-200, +200] | 0 |

Each region uses a double-staggered layer: two sub-layers separated by one tube
radius in Z and half a pitch in Y, giving full azimuthal coverage.

Tube gas and wall volumes are placed as **siblings** in the envelope (not nested)
to avoid Geo2G4 copy-number corruption with shared logical volumes.

### Scintillator tiles

Two 40×40 blocks of 1×1×1 cm³ polystyrene tiles, flush with the tube layer in Z.

## Sensitive Volumes

All sensitive volumes are `GeoFullPhysVol` and are registered with `SHiPUBTManager`:

| Collection | LV name pattern | Count |
|---|---|---|
| Tube gas | `UBT_*_TubeGas_LV` | ~1000+ |
| Tiles | `UBT_Tile_LV` | 3200 |

## Materials


| Material | Density | Usage |
|---|---|---|
| Mylar | 1.39 g/cm³ | Tube wall (15 µm) |
| ArCO2 | 1.842×10⁻³ g/cm³ | Drift gas (70% Ar, 30% CO2) |
| Polystyrene | 1.06 g/cm³ | Scintillator tiles |
| Air | 1.29×10⁻³ g/cm³ | Envelopes |


## Position in World

z = 32720 mm (centre of 32520–32920 mm range, from subsystem_envelopes.csv).

## Geant4 Integration

To register sensitive detectors in a Geant4 application:

```cpp
SHiPUBTManager ubtManager;
UpstreamTaggerFactory factory(materials);
factory.build(&ubtManager);

// After Geo2G4 conversion, iterate the G4LogicalVolumeStore and register
// your SD on volumes whose name contains "TubeGas_LV" or "Tile_LV":
auto* sd = new MyUBTSD("UBTSD");
for (auto* lv : *G4LogicalVolumeStore::GetInstance()) {
    const auto& name = lv->GetName();
    if (name.find("TubeGas_LV") != std::string::npos ||
        name.find("Tile_LV")    != std::string::npos)
        lv->SetSensitiveDetector(sd);
}
```

## Status

- [x] Full segmented geometry (drift tubes + tiles)
- [x] Double-staggered tube layers per region
- [x] GeoFullPhysVol sensitive volume registration via SHiPUBTManager
- [x] New materials (Mylar, ArCO2, Polystyrene) added to SHiPMaterials
- [ ] Verification of tube count against GDML reference
- [ ] SiPM readout geometry
