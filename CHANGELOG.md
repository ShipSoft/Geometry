# Changelog

All notable changes to this project will be documented in this file.

## [0.2.1] - 2026-06-18

### Bug fixes

- *(build)* Include GNUInstallDirs before add_subdirectory(subsystems)
- *(timing)* Install GMX data file and resolve path at runtime
- *(build)* Locate GeoModelXml folder in ubuntu24
### Miscellaneous

- Use shared release workflow from ShipSoft/.github
## [0.2.0] - 2026-06-17

### Features

- *(Calorimeter)* Add configurable calorimeter geometry (#11)
- Install build_geometry and validate_geometry executables
- *(SST)* Add DSST without frame (#19)
- *(build)* Default BUILD_SHARED_LIBS to ON
- *(tileubt)* Replace monolithic slab with tiles (#31)
- *(snd)* Add NeutrinoDetector subsystem (#32)
- *(sbt)* Integrate Surround Background Tagger into DecayVolume (#33)

### Bug fixes

- Correct scintillator density to 1.023 g/cm³ and update TimingDetector geometry
- Use find_package(SQLite3) instead of bare -lsqlite3
- Typo in directory name for licenses
- *(TimeDet)* Correct Z gap between column 2 and columns 1/3
- Install apps/ executables
- *(ci)* Add tomlplusplus dep and use legacy toml.h include

### Refactor

- Add GeoIdentifierTag and adopt hierarchical volume naming
- *(Calorimeter)* Modernise code and compute bar counts at runtime
- *(Calorimeter)* Obtain tomlplusplus via find_package

### Documentation

- Add pre-commit CI badge

### Testing

- Add inter-subsystem consistency tests

### Miscellaneous

- *(ci)* Add gersemi for CMake formatting
- Format CMake with gersemi
- Add GitHub Actions build and test workflow
- Add CI status badge
- Update gersemi hook
- *(build)* Use GNUInstallDirs
- *(build)* Drop pre-project GNUInstallDirs include and add SQLite3::SQLite3 shim
- Build and test with pixi instead of CVMFS
- Use shared ShipSoft/.github reusable pixi-cmake-build workflow
- Add release automation via git-cliff
- Drop redundant tool check in release.sh
## [0.1.0] - 2026-03-11

### Features

- Set up GeoModel project structure
- Initial SHiP geometry implementation

### Miscellaneous

- Prepare v0.1.0 release
