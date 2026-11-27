<!--
SPDX-License-Identifier: LGPL-3.0-or-later
Copyright (C) CERN for the benefit of the SHiP Collaboration
-->

# Repeated volumes

Most of the SHiP geometry is repetition: fibres, bars, tiles and whole layers
that are identical apart from where they sit. How a factory builds that
repetition decides how large the geometry database is, how much memory
GeoModel and Geant4 need, and how fast Geant4 navigates. Building it naively
is easy, and the cost does not show in the factory code. This page describes
how GeoModel and its conversion to Geant4 treat repeated volumes, the
patterns that waste memory, and how to fix them.

The numbers below were measured on the SHiP geometry with GeoModel 6.33 and
Geant4 11.4.3, in #151, #152 and #153.

## How GeoModel and Geant4 store repeated volumes

Every placement in a GeoModel tree is a sequence of nodes added to the
mother: usually a `GeoNameTag`, sometimes a `GeoIdentifierTag`, a
`GeoTransform` and the child physical volume. Each node is a row in the
geometry database and an object in memory, so the cost of a placement is
paid in GeoModel whatever Geant4 does with it.

The GeoModel tools (fullSimLight, gmclash and the others) convert the tree to
Geant4 as follows. The store sizes quoted on this page come from gmclash.

- A physical volume **without children** maps to the Geant4 logical volume
  of its `GeoLogVol`. Childless volumes that share a `GeoLogVol` share one
  Geant4 logical volume, and each placement is a `G4PVPlacement`.
- A physical volume **with children** gets a Geant4 logical volume of its
  own, holding its own copies of all its daughters. Sharing the
  `GeoLogVol` does not help: 84 envelope `GeoPhysVol`s built from 3 shared
  `GeoLogVol`s became 84 Geant4 logical volumes, each with its own fibres.
  Only placing the *same* `GeoPhysVol` several times makes Geant4 share it.
- A `GeoSerialTransformer` is expanded into one `G4PVPlacement` per copy.
  The GeoModel library contains a parameterisation class
  (`Geo2G4STParameterisation`), but the conversion does not use it, so a
  serial transformer saves memory in GeoModel and space in the database, not
  in Geant4. It also means that several serial transformers can share one
  mother, since Geant4 never sees a parameterised volume.

Geant4 still navigates every copy of a shared volume, so sharing leaves the
simulated geometry exactly as it was.

## Anti-pattern 1: a filled volume rebuilt in every slot

A layer, plane or module that has children, and is identical in every slot,
is built anew inside the loop that places it:

```cpp
for (int l = 0; l < nLayers; ++l) {
    auto* env = new GeoPhysVol(envLog);   // new filled volume per layer
    for (int i = 0; i < nBars; ++i) {
        env->add(new GeoNameTag(std::format("L{}_B{}", l, i)));
        env->add(new GeoTransform(barPosition(i)));
        env->add(new GeoPhysVol(barLog));
    }
    mother->add(new GeoNameTag(std::format("layer_{}", l)));
    mother->add(new GeoTransform(layerPosition(l)));
    mother->add(env);
}
```

Every layer adds `nBars` placements to the database, and, because `env` has
children, its own Geant4 logical volume with `nBars` physical volumes.

To fix it, build the filled volume once, before the loop, and place the same
`GeoPhysVol` in every slot. Split the builder into a function that makes the
volume and one that places it:

```cpp
GeoPhysVol* makeLayer(const GeoLogVol* envLog, const GeoLogVol* barLog) {
    auto* env = new GeoPhysVol(envLog);
    for (int i = 0; i < nBars; ++i) {
        env->add(new GeoNameTag(std::format("B{}", i)));
        env->add(new GeoIdentifierTag(i));
        env->add(new GeoTransform(barPosition(i)));
        env->add(new GeoPhysVol(barLog));
    }
    return env;
}

GeoPhysVol* layer = makeLayer(envLog, barLog);
for (int l = 0; l < nLayers; ++l) {
    mother->add(new GeoNameTag(std::format("layer_{}", l)));
    mother->add(new GeoIdentifierTag(l));
    mother->add(new GeoTransform(layerPosition(l)));
    mother->add(layer);
}
```

Look for this pattern wherever a loop over layers, planes or modules creates
a `GeoPhysVol` and then adds children to it. Build one filled volume per
distinct variant (orientation, section size, material) and share it across
every slot of that variant, including across modules: a logical volume
created per module, for example with the module index in its name, prevents
sharing between modules even when the contents are identical.

| Where | Before | After |
|---|---|---|
| Calorimeter HPL casings, 48 slots (#151) | 952,064 Geant4 physical volumes, 105 MB database | 455,448, 20 MB |
| Calorimeter bar layers, 270 slots (#151) | 444,654 Geant4 physical volumes | 413,838 |
| Calorimeter HPL casings placed without their air envelopes, 48 slots (#151) | 413,838 Geant4 physical volumes | 413,790 |
| Neutrino detector fibre planes, 84 slots (#153) | 340,820 Geant4 physical volumes (standalone) | 28,820 |

In the neutrino detector, geantinos along the beam also ran about four times
faster with the shared planes, with identical steps.

## Anti-pattern 2: a new physical volume for every identical child with children

Inside one volume, every copy of a child that has children of its own (a
fibre with a cladding and a core, say) is created with `new GeoPhysVol`:

```cpp
for (int i = 0; i < nFibres; ++i) {
    auto* clad = new GeoPhysVol(cladLog);
    clad->add(new GeoPhysVol(coreLog));
    casing->add(new GeoTransform(fibrePosition(i)));
    casing->add(clad);
}
```

Each `clad` has a child, so Geant4 gets one logical volume per fibre.

To fix it, create the child once and place it at every position:

```cpp
const GeoIntrusivePtr<GeoPhysVol> fibre{new GeoPhysVol(cladLog)};
fibre->add(new GeoPhysVol(coreLog));
for (int i = 0; i < nFibres; ++i) {
    casing->add(new GeoIdentifierTag(i));
    casing->add(new GeoTransform(fibrePosition(i)));
    casing->add(fibre);
}
```

The intrusive pointer releases the fibre if no position uses it. For the HPL
fibres (#151) this cut the Geant4 logical volumes from 15,405 to 4,611. For
children *without* children of their own the gain is only one database row
per copy, since Geant4 already shares their logical volume.

## Anti-pattern 3: a large regular grid placed copy by copy

A grid of identical childless volumes (tiles, fibres) is placed one copy at
a time, each with its own name tag, identifier tag and transform:

```cpp
for (int ix = 0; ix < nX; ++ix) {
    for (int iy = 0; iy < nY; ++iy) {
        env->add(new GeoNameTag(std::format("tile_{}_{}", ix, iy)));
        env->add(new GeoIdentifierTag(ix * nY + iy));
        env->add(new GeoTransform(GeoTrf::Translate3D(x0 + ix * pitch, y0 + iy * pitch, 0)));
        env->add(new GeoPhysVol(tileLog));
    }
}
```

To fix it, describe the grid with one `GeoSerialTransformer`, whose transform
is a function of the copy index, and number the copies with a
`GeoSerialIdentifier`:

```cpp
const GeoGenfun::Variable k;
GeoGenfun::GENFUNCTION iy = GeoGenfun::Mod(nY)(k);
GeoGenfun::GENFUNCTION ix = (k - iy) / nY;
const GeoXF::TRANSFUNCTION xf =
    GeoXF::Pow(GeoTrf::Translate3D(1.0, 0.0, 0.0), x0 + pitch * ix) *
    GeoXF::Pow(GeoTrf::Translate3D(0.0, 1.0, 0.0), y0 + pitch * iy);

env->add(new GeoNameTag("tile"));
env->add(new GeoSerialIdentifier(0));  // copy k gets id k = ix * nY + iy
env->add(new GeoSerialTransformer(new GeoPhysVol(tileLog), &xf, nX * nY));
```

`GeoXF::Pow(step, f)` applies `step` f times, so a unit translation raised to
a function gives a translation by that function's value, in GeoModel units.
Pass plain doubles to GeoGenfun.

This only helps GeoModel and the database (see above): for the 47,000
upstream tagger tiles (#152) the full database fell from 19 MB to 10 MB,
while Geant4 kept the same 47,008 physical volumes. Combine it with sharing
the filled volume (anti-pattern 1) when the grid repeats: the neutrino
detector fibre planes use both.

## Anti-pattern 4: a logical volume and shape for every placement

Every placement creates its own shape and `GeoLogVol`, even when several
placements have the same shape and material:

```cpp
for (const auto& cell : cells) {
    const auto* shape = new GeoTrap(/* cell dimensions */);
    const auto* log = new GeoLogVol(cell.name, shape, material);
    mother->add(new GeoNameTag(cell.name));
    mother->add(new GeoTransform(cell.placement));
    mother->add(new GeoFullPhysVol(log));
}
```

Each `GeoLogVol` becomes a Geant4 logical volume with its own solid, and
GeoModel cannot share them, because only the same object is shared: a second
`GeoLogVol` with the same name, shape and material is a different volume.

To fix it, create one shape and logical volume per distinct shape and
material, keyed for example by the cell's dimensions, and reuse it for every
placement that matches. Put the identity of each placement in its name tag
and copy number rather than in the logical-volume name.

The gain is the number of duplicate shapes, usually far smaller than in the
patterns above. In the decay volume, 3,713 logical volumes hold 2,279
distinct shapes, since the vessel tapers and most sensor cells differ in
size along z. Sharing the rest would probably save less than a megabyte.
There, the logical-volume names also carry each sensor cell's identity
(`sensors_Side_S3_XP_C2_P1_W0`, say), so sharing would be a breaking change
for little gain, and the decay volume was left as it is.
Keep the pattern in mind for new subsystems, where getting it right from the
start costs nothing.

## Identifying volumes after sharing

A shared volume has one set of children, so the names and copy numbers of
those children can no longer say which layer or module they are in.
Identify a volume by its own name and copy number within the shared volume,
and take the layer and module from the placements above it, through the
touchable history in Geant4:

- Give every repeated child a copy number (`GeoIdentifierTag`, or
  `GeoSerialIdentifier` for a serial transformer), and document the
  numbering. Do not rely on names alone.
- Keep the placement of each shared volume uniquely named and numbered:
  that is where the layer and module now live. Make the copy number unique
  across the whole subsystem, not just within a module, and provide a
  function that decodes it. In the calorimeter (#151),
  `calo::layerCopyNumber(module, layer)` is `module * 1000 + layer`. A
  number that restarts in every module leaves the module recoverable only
  by parsing the name.
- Place the shared volume directly, without a wrapper around it. An air
  envelope of the same size adds a level and a volume, and the copy number
  ends up on the wrapper rather than on the volume that holds the children. In
  #151 the HPL casings were wrapped and the bar layers were not, so the
  layer sat two levels above a fibre's cladding but one level above a bar.
- Document the depth at which each index lives, counted from the step
  point, so that a sensitive detector can use `GetCopyNumber(depth)`
  directly.
- Warn the people who write sensitive detectors and digitisers. One Geant4
  physical volume now stands for every slot, so a lookup cached by
  `G4VPhysicalVolume*`, or keyed on the step volume's name, returns the
  first slot's layer and module for every hit, and nothing shows that it
  went wrong. Hits have to be identified through the touchable history
  (`GetCopyNumber(depth)` / `GetVolume(depth)`).
- Shared slots cannot differ from each other. Misaligning one layer or
  marking dead channels in it means giving that layer a volume of its own.
- Changing names that the simulation or digitisation match on is a breaking
  change. Mark the commit with `!` and say in the pull request what changed,
  so that the downstream code can be checked.

## Checking a change

### Count what Geant4 holds

gmclash loads the geometry into Geant4 and
prints the store sizes and the size of the expanded tree:

```sh
build_geometry --standalone NeutrinoDetector -o snd.db
gmclash -g snd.db -n 10 -o snd.json | grep -E "Size of G4|Number of volumes"
```

`G4PhysicalVolumeStore` and `G4LogicalVolumeStore` should fall, while
`Number of volumes ... in tree` stays the same: Geant4 must still see every
volume. The overlap check (`ctest -R OverlapCheck`) should report the same
clashes.

### Check that navigation is unchanged

Fire geantinos with fixed seeds
through the old and the new geometry with fullSimLight. Geantinos have no
physics, so they measure navigation alone, and with the same seeds the
number of steps and the track length per event in the run summary must be
identical:

```text
/run/numberOfThreads 1
/random/setSeeds 12345 67890
/FSLdet/setField 0 tesla
/run/initialize
/FSLgun/particle geantino
/FSLgun/energy 1 GeV
/FSLgun/position 13 17 1800 mm
/FSLgun/primaryPerEvt 1
/run/beamOn 50000
```

Run it with `fullSimLight -g snd.db -m geantino.mac` (add `-p` to time the
event loop without user actions, which also drops the step summary). Without
`/FSLgun/direction` the gun fires in a random direction each event; set a
fixed direction to send every track along the beam or through a layer at a
grazing angle. `/FSLdet/setField` is needed before `/run/initialize` with
GeoModel releases that lack the fix for #90.

### Compare times fairly

On a shared machine, alternate the old and the new
geometry over several rounds and compare the shortest runs. Report store
sizes and database contents as exact, and times as indicative.
