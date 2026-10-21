# SHiP Geometry

[![Build and Test](https://github.com/ShipSoft/Geometry/actions/workflows/build-test.yml/badge.svg)](https://github.com/ShipSoft/Geometry/actions/workflows/build-test.yml)

This repository contains the SHiP experiment geometry implementation using GeoModel.

The SHiP geometry is described using GeoModel and is used by the simulation and digitisation/reconstruction packages which are developed in a separate repository.

## Documentation

An [automatic class reference](https://shipsoft.github.io/Geometry/) is built using Doxygen from comments in the C++ code.

## Coordinate System

- **Origin**: target front face (first tungsten disk)
- **Z-axis**: along beam direction (positive downstream)
- **Y-axis**: vertical (against gravity)
- **X-axis**: horizontal, perpendicular to beam (completes right-handed system)
- **Units**: typed `mp-units` quantities in code; mm (GeoModel native) and
  radians at the GeoModel API
- **Beam axis height**: 1.7 m above floor

### Unit convention

Every dimensional constant (length, angle, density, molar mass) is a typed
[mp-units](https://mpusz.github.io/mp-units/) quantity, declared in the unit
its value is written in: `2160.0 * units::mm`, `80.0 * units::cm`,
`2.3 * units::deg`. The shared vocabulary lives in
`include/SHiP/geometry/Units.h` (`SHiP::geometry::units`).

GeoModel's API takes raw doubles in its native units (mm, radians).
`units::gm()` is the single bridge: it multiplies a quantity's stored value
by the matching `GeoModelKernelUnits` factor — the identical arithmetic the
former `value * GeoModelKernelUnits::<unit>` literals performed — so the
typed constants are numerically free and the geometry is unchanged bit for
bit. Raw doubles appear only (a) at `gm()`-wrapped GeoModel/GeoTrf calls and
(b) in genuinely unitless math (growth slopes, direction cosines, counts,
GeoGenfun/Eigen inputs), converted as early as possible.

(Subsystems are converted incrementally; unconverted ones still use the
`GeoModelKernelUnits` annotation style described above until their
milestone lands.)

## Implementation Status

| Subsystem | Status | Description |
|-----------|--------|-------------|
| [Cavern](subsystems/Cavern/README.md) | Complete | World volume with subtracted rock cavities |
| [Target](subsystems/Target/README.md) | Complete | 2026 BDF target: 33 W disks in grooved steel core with jacket and endcap |
| [MuonShield](subsystems/MuonShield/README.md) | Approximate | 6 stations, box approximations of arb8 shapes |
| [NeutrinoDetector](subsystems/NeutrinoDetector/README.md) | Approximate | Veto + Si/W target + HCAL with individual scintillating fibres |
| [Magnet](subsystems/Magnet/README.md) | Approximate | Iron yoke with box-shaped coils (should be tubes) |
| [DecayVolume](subsystems/DecayVolume/README.md) | Implemented | Frustum: SBT steel structure + LAB sensors + helium centre |
| [TimingDetector](subsystems/TimingDetector/README.md) | Complete | 330 scintillator bars via GeoModelXML |
| [UpstreamTagger](subsystems/UpstreamTagger/README.md) | Simulation-ready | Segmented 4.4 × 6.4 m tile plane, 47000 polystyrene tiles (fine 20 mm + coarse 40 mm) |
| [Trackers](subsystems/Trackers/README.md) | Complete | 4 stations, 4 stereo views each, 9600 straw tubes |
| [Calorimeter](subsystems/Calorimeter/README.md) | Simulation-ready | ECAL + HCAL sampling layers (Pb/PVT/HPL + Fe/PVT) |

## Building

### Prerequisites

- CMake 3.16 or later
- C++23 compiler (CI builds with the GCC pinned in `pixi.toml`)
- GeoModel libraries (GeoModelCore, GeoModelIO, GeoModelTools) version 6.22+
- Catch2 3 (tests only; configure with `-DBUILD_TESTING=OFF` to skip them)

### Build Instructions

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests
ctest --test-dir build
```

On x86_64, the `OverlapCheck` test runs `gmclash` on the built geometry and compares its
report with `tests/known_clashes.json`. It fails on a new overlap, on a known
one that got deeper, and on a known one that has gone away. It takes a few
minutes; `ctest --test-dir build -LE overlap` skips it. After an intended
change, refresh the list and give every new entry an issue number:

```bash
python3 tests/check_clashes.py build/gmclash_report.json tests/known_clashes.json --update
```

### Build Geometry

```bash
# Build the complete SHiP geometry
./build/apps/build_geometry [-o output_file.db]

# Assemble a selection of subsystems into the world
./build/apps/build_geometry <Name> <Name> ... [-o output_file.db]

# Build a single subsystem on its own, in its local frame
./build/apps/build_geometry --standalone <Name> [-o output_file.db]

# List the available subsystem names
./build/apps/build_geometry --list

# View in gmex
gmex output_file.db
```

With no subsystem named, the complete detector is built. Naming subsystems (as
spelled by `--list`, e.g. `Calorimeter`) assembles those into the world at their
declared placements. `--standalone` instead builds exactly one named subsystem
by itself, in its own local frame at the origin, with no world around it.

The output file is set with `-o`; the default is `ship_geometry.db` for the full
build, `ship_selection.db` for a selection, and `<Name>.db` with `--standalone`.

### Installing

```bash
cmake --install build --prefix /path/to/install
```

This installs the headers, libraries, and `SHiPGeometryConfig.cmake` so that
downstream projects can locate the package:

```cmake
find_package(SHiPGeometry CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE SHiP::Geometry)
```

The package config calls `find_dependency` for GeoModelCore, GeoModelIO,
GeoModelTools, and mp-units automatically. mp-units 2.5 (header-only, from
the ship conda channel) is a public dependency: the installed headers use
its quantity types.

## Architecture

### Factory Pattern

Each subsystem is a self-contained factory class that builds its geometry,
describes where it belongs, and registers itself:

```cpp
class FooFactory {
public:
    explicit FooFactory(Materials& materials);
    GeoPhysVol* build();

    // Self-description consumed by the assembler:
    // name, tree node, id, placement (x/y/z mm), and whether it is the world.
    static SubsystemDescriptor descriptor() {
        return {"Foo", "/SHiP/foo", 42, 0.0, 0.0, 12345.0, /*isWorld=*/false};
    }
private:
    Materials& m_materials;
};
```

with a single registration line in the factory's `.cpp` (inside
`namespace SHiP::geometry`):

```cpp
REGISTER_SUBSYSTEM(FooFactory)
```

The assembler names no subsystem. `assembleGeometry()`, also reachable via
`DetectorBuilder::build()`, iterates the registry, builds the world (the subsystem whose descriptor sets `isWorld`, i.e. the Cavern), and
places every other registered subsystem at its declared position, ordered by
`(z, id)`. A single subsystem can be built on its own, in its local frame,
with `buildSubsystem("Foo")`. The descriptor type lives in
`include/SHiP/geometry/SubsystemDescriptor.h`, and the registry and macro in
`include/SHiP/geometry/SubsystemRegistry.h`.

### Materials

All materials are managed centrally by `Materials`. To use an existing
material in a factory:

```cpp
const GeoMaterial* iron = m_materials.requireMaterial("Iron");
```

To add a new material, edit `src/Materials.cpp`:
1. Add elements in `createElements()` if not already present
2. Add the material in `createMaterials()` with composition and density
3. Call `material->lock()` after defining the composition

## Adding or Modifying a Subsystem

1. **Header**: `subsystems/<Name>/include/SHiP/geometry/<Name>/<Name>Factory.h`
   — declare the factory class with dimension constants as `static constexpr`
   members, plus `static SubsystemDescriptor descriptor()` returning its name,
   tree node, id, and placement
2. **Implementation**: `subsystems/<Name>/src/<Name>Factory.cpp` — implement
   `build()` using GeoModel primitives (`GeoBox`, `GeoTubs`, `GeoLogVol`,
   `GeoPhysVol`, `GeoTransform`, etc.)
3. **Registration**: add one line, `REGISTER_SUBSYSTEM(<Name>Factory)`, at
   the end of the factory `.cpp`, inside `namespace SHiP::geometry`. The
   subsystem registers itself; **do not** edit `src/DetectorBuilder.cpp`, which
   names no subsystem.
4. **CMake**: add sources/headers to `subsystems/<Name>/CMakeLists.txt`, and
   add `<Name>` to `SHIP_SUBSYSTEMS` in the top-level `CMakeLists.txt`. That
   list drives the subdirectory walk, the install rules, and the libraries
   `ship_geometry` links, and `test_registry` requires every entry to have
   registered itself under that name. `ship_geometry` also calls the empty
   `<Name>Factory_linkAnchor()` that `REGISTER_SUBSYSTEM` defines, so the
   linker keeps the library and its registration. The factory therefore has to
   be called `<Name>Factory`; otherwise `ship_geometry` fails to link.
5. **Docs**: update the subsystem `README.md` with geometry tree, materials,
   and status

## Structure

```text
geometry/
├── include/SHiP/geometry/  # Public headers (DetectorBuilder, Materials, SubsystemRegistry)
├── src/                     # Core implementation
├── subsystems/              # Detector subsystem factories
│   ├── Cavern/
│   ├── Target/
│   ├── MuonShield/
│   ├── NeutrinoDetector/
│   ├── Magnet/
│   ├── DecayVolume/
│   ├── Trackers/
│   ├── Calorimeter/
│   ├── UpstreamTagger/
│   └── TimingDetector/
├── apps/                    # build_geometry, validate_geometry
├── tests/                   # Unit tests
└── cmake/                   # CMake package config
```

## Reference GDML

The reference GDML exported from FairShip is not tracked in this repository
(ignored via `.gitignore`). To obtain it, export from FairShip and place in
`gdml/`. The `gdml2gm` tool does not support GDML assemblies, so direct
conversion is not possible — the GDML serves as a numerical reference for
geometry parameters during C++ implementation.

## License

The SHiP geometry is distributed under the GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later). See the [LICENSE](LICENSE) file for details.

Copyright is held by CERN for the benefit of the SHiP Collaboration. Some components are distributed under different licenses and copyrights — see the individual file headers and the [LICENSES](LICENSES/) directory for details. This project follows the [REUSE specification](https://reuse.software/) for licensing information.
