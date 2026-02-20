# SHiP Geometry

This repository contains the SHiP experiment geometry implementation using GeoModel.

The SHiP geometry is described using GeoModel and is used by the simulation and digitization/reconstruction packages which are developed in a separate repository.

## Building

### Prerequisites

- CMake 3.16 or later
- C++17 compatible compiler
- GeoModel libraries (GeoModelCore, GeoModelKernel, GeoModelIO)

### Build Instructions

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests (when available)
ctest --test-dir build
```

### Build Geometry

```bash
# Build the complete SHiP geometry
./build/apps/build_geometry [output_file.db]
```

## Structure

- `src/` - Core geometry implementation
- `include/` - Public headers
- `subsystems/` - Individual detector subsystem implementations
- `apps/` - Applications and utilities
- `tests/` - Unit tests
- `gdml/` - Reference GDML files

## Subsystems

The SHiP detector consists of the following subsystems:

1. **Cavern** - Experimental hall (ECN3)
2. **Target** - Proton target and shielding
3. **MuonShield** - Muon shield magnets
4. **Magnet** - Spectrometer magnet
5. **DecayVolume** - Decay vessel and veto system
6. **Trackers** - Straw tube tracking stations
7. **Calorimeter** - Electromagnetic calorimeter
8. **UpstreamTagger** - Upstream veto tagger
9. **TimingDetector** - Timing detector

## License

The SHiP geometry is distributed under the GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later). See the [LICENSE](LICENSE) file for details.

Copyright is held by CERN for the benefit of the SHiP Collaboration. Some components are distributed under different licenses and copyrights - see the individual file headers and the [LICENSES](LICENSES/) directory for details. This project follows the [REUSE specification](https://reuse.software/) for licensing information.
