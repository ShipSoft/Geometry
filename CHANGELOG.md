# Changelog

All notable changes to this project will be documented in this file.

## [0.3.0] - 2026-10-20

### Features

- *(doxygen)* Add API documentation via Doxygen and GitHub Pages
- Add pixi task to export geometry to GDML
- Enable consumption as a pixi source dependency
- *(target)* Implement 2026 BDF target design
- *(target)* Close the 2026 target vessel upstream
- *(tileubt)* Tile the full 4.4 x 6.4 m tagger
- *(materials)* Look materials up by string_view
- *(apps)* Build individual subsystems from build_geometry
- *(apps)* [**breaking**] Set the output with -o and the frame with --standalone
- [**breaking**] Adopt mp-units for typed dimensional constants
- *(checks)* Add constexpr geometry predicates and EDMS envelopes
- *(trackers)* Check the station internals at compile time
- *(neutrinodetector)* Check the SND fits its container at compile time
- *(upstreamtagger)* Check the tile regions at compile time
- *(muonshield)* Check the station table at compile time
- *(target)* Check the 2026 target layout at compile time
- *(magnet)* Check the magnet parts at compile time
- *(timingdetector)* Check the bar grid at compile time
- *(cavern)* Check that every subsystem sits in a hall
- *(decayvolume)* Check the shipped SBT wall stack at compile time
- *(cavern)* Check containers against the halls taken together
- *(layout)* Bound the known deviations and overlaps
- *(magnet,muonshield)* Bound the known overlaps and protrusions

### Bug fixes

- Unroll install-hooks loop so deno_task_shell can parse it
- Make SBT beam placement frames right-handed
- *(decayvolume)* Derive helium from the SBT surfaces directly
- *(release)* Roll back on any failure, fail on unbumpable versions
- Let a failed git diff fail the clang-tidy-diff task
- *(release)* Tag the release only after it lands on main
- *(cavern)* Place the TCC8 tunnel and ECN3 hall at the layout positions
- *(cavern)* End the TCC8 floor at 21.37 m
- *(cavern)* Take the tunnel and hall cross-sections from the CAD model
- *(geometry)* Validate before building and free the world on failure

### Refactor

- Share subsystem CMake recipe and placement/config helpers
- Build TimingDetector geometry in C++ instead of GeoModelXML
- *(target)* Share He-centre constant and tie disk array to s_numDisks
- *(Calorimeter)* [**breaking**] Define the geometry as compile-time constants
- *(DecayVolume)* [**breaking**] Define the SBT as compile-time constants
- [**breaking**] Drop the TOML configuration plumbing
- *(apps)* Log build_geometry diagnostics with spdlog
- *(units)* Retype TimingDetector, Magnet, placements and materials
- *(units)* Retype UpstreamTagger, NeutrinoDetector and MuonShield
- *(units)* Retype the Calorimeter
- *(units)* Retype the Trackers
- *(units)* Retype the Target
- *(units)* Retype the DecayVolume SBT
- *(layout)* Check subsystem placements in Layout.h
- *(calorimeter)* Pin the stack depth with a static_assert
- [**breaking**] Move the geometry into the SHiP namespace
- [**breaking**] Lower_case the namespaces nested in SHiP::geometry
- Take the namespace rule from the shared clang-tidy config
- *(upstream-tagger)* Remove UpstreamTaggerManager

### Documentation

- Drop pre-commit.ci badge from README
- Link Doxygen API reference from README
- Correct Magnet container-dimensions comment
- Add CONTRIBUTING with prek hook setup
- Adopt AI policy
- Describe what the clang-tidy gate actually does, and fix the local check
- *(registry)* Document the public API
- *(Target)* Update the usage snippet to the SHiP::geometry API
- Use the lower_case namespaces and the SHiP/geometry header path

### Performance

- Place identical straws and fibres with GeoSerialTransformer
- Share one fibre physical volume across HCal planes

### Styling

- Make every dimensional literal carry an explicit unit
- *(tileubt)* Apply clang-tidy fixes to the tagger factory

### Testing

- Check the placement of every subsystem in PositionsSanity
- Cover resolveConfigPath and reject directories in fallback chain
- Assert all placement rotations preserve handedness
- Report full placement path in handedness check
- *(target)* Assert jacket, flanges, endcap and cover-plate operands
- *(cavern)* Probe floors, walls and pits of the excavation
- *(registry)* Cover subsystem registration and assembly
- *(decayvolume)* Drop the container-size test
- Rename OverlapCheck to BuildAndReadBack
- *(registry)* Check the z-tie order and free what the tests build

### Miscellaneous

- Add monthly pixi lock file update workflow
- Update pixi lock file
- Publish ship_geometry.db as a build artefact
- Lint with prek via pixi
- Enable Renovate via shared preset
- Remove Dependabot in favour of Renovate
- Update pixi lock file
- Enforce conventional commits in CI via commit-check
- Add concurrency control, shared-config sync and citation metadata
- Remove dead code and modernize idioms
- Sync shared configs
- Update pixi lock file
- Grant config-sync job explicit permissions
- Update pixi lock file
- Sync shared configs
- Gate merges on uniform aggregator checks
- Check C++ against the Core Guidelines with clang-tidy
- Build and test on linux-aarch64
- Opt in to the org-wide forbid-ship-namespace hook

### Build

- *(pixi)* Add eigen-abi-devel and nlohmann_json
- Pass CMAKE_PREFIX_PATH to the configure task
- Get Catch2 from conda-forge instead of FetchContent
- Resolve SHiP packages from ship-experiment/stable
- Keep subsystem libraries linked through link anchors
## [0.2.1] - 2026-06-18

### Bug fixes

- *(build)* Include GNUInstallDirs before add_subdirectory(subsystems)
- *(timing)* Install GMX data file and resolve path at runtime

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
