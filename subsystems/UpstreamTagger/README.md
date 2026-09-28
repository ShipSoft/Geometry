# UpstreamTagger

Upstream background tagger (UBT).

## Description

The UpstreamTagger sits just upstream of the decay vessel and tags charged
particles entering from the target/muon-shield region. It is implemented as a
single plane of polystyrene scintillator **tiles**, coplanar in Z, replacing
the previous monolithic slab.

Two tile granularities are used:

- **Fine** tiles — 20 × 20 mm² face, 20 mm pitch — in the central beam region
  and the two side extensions, where occupancy is highest.
- **Coarse** tiles — 40 × 40 mm² face, 40 mm pitch — in the outer bands and the
  central horizontal strip.

The tiles are grouped into seven abutting regions (origin at the plane centre;
X horizontal, Y vertical, Z along the beam):

```
   Y
   ^
+3200 +------+------------------------------------+------+
      | EXT  |        COARSE band (top)           | EXT  |  y=[+200,+3200]
+200  |      +--------+------------------+--------+      |
      | EXT  |  FINE  |  COARSE central  |  FINE  | EXT  |  y=[-200,+200]
-200  |      +--------+------------------+--------+      |
      | EXT  |        COARSE band (bottom)        | EXT  |  y=[-3200,-200]
-3200 +------+------------------------------------+------+
     -2200  -1000   -600              +600    +1000   +2200  -> X
```

Fine tiles are 5 mm thick and coarse tiles 10 mm thick; all are centred on
the same Z plane at the container centre. The plane
spans X ∈ [−2200, +2200] mm, Y ∈ [−3200, +3200] mm: the tiles cover the full
4.4 × 6.4 m container cross-section.

Like the calorimeter bar layers, the individual tiles are `GeoPhysVol` with
hierarchical `/SHiP/upstream_tagger/...` names; sensitive-detector assignment
is done downstream by name pattern. The whole assembly is returned as a single
`GeoFullPhysVol` container so the tagger keeps a sensitive tree-top, which is
registered with `SHiPUBTManager`.

## Geometry tree

```
/SHiP/upstream_tagger (GeoFullPhysVol, Air, 4400 × 6400 × 160 mm)
 ├─ fine_left       (Air envelope)  → 20×20  fine   tiles   (Polystyrene)
 ├─ fine_right      (Air envelope)  → 20×20  fine   tiles
 ├─ coarse_central  (Air envelope)  → 30×10  coarse tiles
 ├─ coarse_top      (Air envelope)  → 50×75  coarse tiles
 ├─ coarse_bottom   (Air envelope)  → 50×75  coarse tiles
 ├─ ext_left        (Air envelope)  → 60×320 fine   tiles
 └─ ext_right       (Air envelope)  → 60×320 fine   tiles
```

Tile totals: fine 2 × 400 + 2 × 19 200 = 39 200; coarse 300 + 2 × 3 750 = 7 800;
**47 000 tiles** in total.

Position in world: z = 32 720 mm (centre of the 32.52–32.92 m envelope).

## Materials

| Material    | Density    | Usage                          |
|-------------|------------|--------------------------------|
| Polystyrene | 1.05 g/cm³ | Scintillator tiles             |
| Air         | 1.29 mg/cm³| Container and region envelopes |

Both materials are already present in the central `SHiPMaterials` catalogue
(Polystyrene was added with the calorimeter, Air is a base material), so this
subsystem adds none — consistent with the straw tracker, which likewise reuses
the shared catalogue.

## Tests

`test_upstreamtagger.cpp` exercises:

- `UpstreamTaggerWithinEnvelope` — the container is a `GeoBox` within the CSV
  envelope limits (halfX ≤ 2200, halfY ≤ 3200, halfZ ≤ 200 mm).
- `UBTHasSensitiveVolume` — the returned container is a `GeoVFullPhysVol`.
- `UBTHasSevenRegions` — the container holds the seven tile regions.
- `UBTTileCount` — the regions contain 47 000 tiles in total.

## Status

- [x] C++ implementation (tile-segmented plane)
- [x] Fine / coarse dual-granularity tiling
- [x] `GeoFullPhysVol` container registered with `SHiPUBTManager`
- [ ] Per-tile SiPM readout geometry
- [ ] Register tiles as sensitive volumes in the simulation (currently by name)
- [ ] Verification against GDML / TDR tile map

## TODO

- Add SiPM / fibre readout geometry per tile.
- Confirm the tile granularity map (fine vs. coarse regions) against the UBT
  technical design once finalised.
- Verify the tile-plane Z position and thickness against the GDML reference.
