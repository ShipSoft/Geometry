# SHiP Geometry

[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/ShipSoft/Geometry/main.svg)](https://results.pre-commit.ci/latest/github/ShipSoft/Geometry/main) [![Build and Test](https://github.com/ShipSoft/Geometry/actions/workflows/build-test.yml/badge.svg)](https://github.com/ShipSoft/Geometry/actions/workflows/build-test.yml)

This repository contains the SHiP experiment geometry implementation using GeoModel.

The SHiP geometry is described using GeoModel and is used by the simulation and digitisation/reconstruction packages which are developed in a separate repository.

## Coordinate System

- **Origin**: target front face (first tungsten disk)
- **Z-axis**: along beam direction (positive downstream)
- **Y-axis**: vertical (against gravity)
- **X-axis**: horizontal, perpendicular to beam (completes right-handed system)
- **Units**: mm (GeoModel native), angles in radians
- **Beam axis height**: 1.7 m above floor

## Implementation Status

| Subsystem | Status | Description |
|-----------|--------|-------------|
| Cavern | Complete | World volume with subtracted rock cavities |
| Target | Complete | 19 W slabs with Ta cladding in Inconel vessel |
| MuonShield | Approximate | 6 stations, box approximations of arb8 shapes |
| Magnet | Approximate | Iron yoke with box-shaped coils (should be tubes) |
| DecayVolume | Approximate | Rectangular vessel (should be frustum) |
| TimingDetector | Complete | 330 scintillator bars via GeoModelXML |
| UpstreamTagger | Approximate | Monolithic slab (needs bar segmentation) |
| Trackers | Envelope only | 4 empty station boxes |
| Calorimeter | Envelope only | 3 empty component boxes (ECAL front/back, HCAL) |

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

# View in gmex
gmex output_file.db
```

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

Each subsystem is implemented as a factory class:

```cpp
class FooFactory {
public:
    explicit FooFactory(SHiPMaterials& materials);
    GeoPhysVol* build();
private:
    SHiPMaterials& m_materials;
};
```

`SHiPGeometryBuilder::build()` orchestrates all factories, creating the world
volume (Cavern) and placing each subsystem at its global z-position.

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
   the factory class with dimension constants as `static constexpr` members
2. **Implementation**: `subsystems/<Name>/src/<Name>Factory.cpp` — implement
   `build()` using GeoModel primitives (`GeoBox`, `GeoTubs`, `GeoLogVol`,
   `GeoPhysVol`, `GeoTransform`, etc.)
3. **Registration**: add a `build()` + placement call in
   `src/SHiPGeometry.cpp` (`SHiPGeometryBuilder::build()`)
4. **CMake**: add sources/headers to `subsystems/<Name>/CMakeLists.txt`
5. **Docs**: update the subsystem `README.md` with geometry tree, materials,
   and status

## Structure

```
geometry/
├── include/SHiPGeometry/   # Public headers (SHiPGeometry, SHiPMaterials)
├── src/                     # Core implementation
├── subsystems/              # Detector subsystem factories
│   ├── Cavern/
│   ├── Target/
│   ├── MuonShield/
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
