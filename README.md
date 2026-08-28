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
- **Units**: mm (GeoModel native), angles in radians
- **Beam axis height**: 1.7 m above floor

### Unit convention

Every dimensional literal (length or angle) in the code carries an explicit
`GeoModelKernelUnits` unit, e.g. `3000.0 * GeoModelKernelUnits::mm` or
`90.0 * deg` — either at its definition or, for config-driven values with an
`_mm` suffix, at the point of conversion into GeoModel units. GeoModel's
native length unit is mm (`GeoModelKernelUnits::mm == 1.0`), so the
annotations are numerically free; they exist to make the unit of every
quantity explicit at the point where it is written.

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
| [UpstreamTagger](subsystems/UpstreamTagger/README.md) | Simulation-ready | Segmented tile plane, 16300 polystyrene tiles (fine 20 mm + coarse 40 mm) |
| [Trackers](subsystems/Trackers/README.md) | Complete | 4 stations, 4 stereo views each, 9600 straw tubes |
| [Calorimeter](subsystems/Calorimeter/README.md) | Simulation-ready | ECAL + HCAL sampling layers driven by `calo.toml` (Pb/PVT/HPL + Fe/PVT) |

## Building

### Prerequisites

- CMake 3.16 or later
- C++20 compatible compiler
- GeoModel libraries (GeoModelCore, GeoModelIO, GeoModelTools) version 6.22+

### Build Instructions

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests
ctest --test-dir build
```

### Build Geometry

```bash
# Build the complete SHiP geometry
./build/apps/build_geometry [output_file.db]

# Build a single subsystem on its own
./build/apps/build_geometry <Name> [output_file.db]

# List the available subsystem names
./build/apps/build_geometry --list

# View in gmex
gmex output_file.db
```

With no subsystem named, the complete detector is built. Naming a subsystem
(as spelled by `--list`, e.g. `Calorimeter`) builds just that subsystem in its
own local frame. A token ending in `.db` is taken as the output file; the
default is `ship_geometry.db` for the full build, or `<Name>.db` for a single
subsystem.

### Installing

```bash
cmake --install build --prefix /path/to/install
```

This installs the headers, libraries, and `SHiPGeometryConfig.cmake` so that
downstream projects can locate the package:

```cmake
find_package(SHiPGeometry CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE SHiPGeometry::SHiPGeometry)
```

The package config calls `find_dependency` for GeoModelCore, GeoModelIO, and
GeoModelTools automatically.

## Architecture

### Factory Pattern

Each subsystem is a self-contained factory class that builds its geometry,
describes where it belongs, and registers itself:

```cpp
class FooFactory {
public:
    explicit FooFactory(SHiPMaterials& materials);
    GeoPhysVol* build();

    // Self-description consumed by the assembler:
    // name, tree node, id, placement (x/y/z mm), and whether it is the world.
    static SubsystemDescriptor descriptor() {
        return {"Foo", "/SHiP/foo", 42, 0.0, 0.0, 12345.0, /*isWorld=*/false};
    }
private:
    SHiPMaterials& m_materials;
};
```

with a single registration line in the factory's `.cpp` (inside
`namespace SHiPGeometry`):

```cpp
REGISTER_SUBSYSTEM(FooFactory)
```

The assembler names no subsystem. `assembleGeometry()` — also reachable via
the unchanged `SHiPGeometryBuilder::build()` — iterates the registry, builds
the world (the subsystem whose descriptor sets `isWorld`, i.e. the Cavern),
and places every other registered subsystem at its declared position, ordered
by `(z, id)`. A single subsystem can be built on its own, in its local frame,
with `buildSubsystem("Foo")`. The registry, descriptor type, and macro live in
`include/SHiPGeometry/SubsystemRegistry.h`.

### Materials

All materials are managed centrally by `SHiPMaterials`. To use an existing
material in a factory:

```cpp
const GeoMaterial* iron = m_materials.requireMaterial("Iron");
```

To add a new material, edit `src/SHiPMaterials.cpp`:
1. Add elements in `createElements()` if not already present
2. Add the material in `createMaterials()` with composition and density
3. Call `material->lock()` after defining the composition

## Adding or Modifying a Subsystem

1. **Header**: `subsystems/<Name>/include/<Name>/<Name>Factory.h` — declare
   the factory class with dimension constants as `static constexpr` members,
   plus `static SubsystemDescriptor descriptor()` returning its name, tree
   node, id, and placement
2. **Implementation**: `subsystems/<Name>/src/<Name>Factory.cpp` — implement
   `build()` using GeoModel primitives (`GeoBox`, `GeoTubs`, `GeoLogVol`,
   `GeoPhysVol`, `GeoTransform`, etc.)
3. **Registration**: add one line — `REGISTER_SUBSYSTEM(<Name>Factory)` — at
   the end of the factory `.cpp`, inside `namespace SHiPGeometry`. The
   subsystem registers itself; **do not** edit `src/SHiPGeometry.cpp`, which
   names no subsystem.
4. **CMake**: add sources/headers to `subsystems/<Name>/CMakeLists.txt`, and
   add the new library to the subsystem list linked by `SHiPGeometry` in
   `src/CMakeLists.txt`. For shared-library builds (the only supported
   configuration) that target applies `-Wl,--no-as-needed` as an INTERFACE
   link option, so the library stays in each consumer's `DT_NEEDED` and its
   `REGISTER_SUBSYSTEM` initializer is not dropped by the linker's default
   `--as-needed`. Static builds are not supported.
5. **Docs**: update the subsystem `README.md` with geometry tree, materials,
   and status

## Structure

```
geometry/
├── include/SHiPGeometry/   # Public headers (SHiPGeometry, SHiPMaterials, SubsystemRegistry)
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
