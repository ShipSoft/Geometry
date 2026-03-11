# UpstreamTagger

Upstream veto tagger.

## Description

The UpstreamTagger subsystem implements a scintillator slab upstream of the decay vessel, used to veto charged particles entering from the target region. Currently modelled as a single monolithic scintillator box. The full implementation requires bar segmentation with SiPM readout.

The volume is created as a `GeoFullPhysVol` (rather than `GeoPhysVol`) to allow sensitive detector registration via `SHiPUBTManager`.

## Geometry Tree

```
Upstream_Tagger (Scintillator, 4400×6400×160 mm)
```

Position in world: z = 32720 mm.

## Materials

| Material     | Density    | Usage           |
|--------------|------------|-----------------|
| Scintillator | 1.032 g/cm³ | Detector slab  |

## Status

- [x] C++ implementation (monolithic slab)
- [ ] Implement bar segmentation with SiPM readout
- [ ] Verification against GDML

## TODO

- Implement individual scintillator bar segmentation
- Add SiPM readout geometry
- Register individual bars as sensitive volumes (currently the whole slab is one volume)
- Verify slab dimensions against GDML reference
