# Contributing to SHiP Geometry

Thank you for your interest in contributing! As part of the SHiP Collaboration, we follow a set of standards to ensure code quality and maintainability.

## Development Workflow

1. **Fork and Clone**: Create a fork of the repository and clone it locally.
2. **Environment**: Install [pixi](https://pixi.sh) — it provisions all build dependencies (GeoModel 6.22+, a recent CMake/Ninja/compiler) from `conda-forge` and [`prefix.dev/ship`](https://prefix.dev/channels/ship). Then `pixi install` once to materialise the environment.
3. **Pre-commit Hooks**: We use [`prek`](https://github.com/j178/prek) (a drop-in `pre-commit` replacement) to enforce coding standards. The hook tools come from the pixi `lint` environment, so versions are tracked in `pixi.lock` and run identically everywhere. Install the hooks once:
   ```bash
   pixi run install-hooks
   ```
   Run all hooks manually at any time with `pixi run lint`.
4. **Branching**: Create a feature branch for your changes.
5. **Coding Standards**:
   - Follow the existing C++ style (formatting is enforced by `clang-format`).
   - C++ is checked against the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
     by `clang-tidy`, using the shared `.clang-tidy` synced from
     [ShipSoft/.github](https://github.com/ShipSoft/.github/tree/main/sync). Only
     findings on lines your patch changes are reported, so you are never asked to
     clean up code you did not touch. Whether those findings fail the build or are
     reported for information is set by `fail-on-new` in
     `.github/workflows/clang-tidy.yml`. Check the sources you changed locally
     with:
     ```bash
     pixi run clang-tidy-diff
     ```
     That task cannot see a header-only change, because headers have no compile
     command of their own; CI reaches them through the sources that include them.
     After touching a header, run the whole tree with `pixi run clang-tidy` or
     leave it to CI.
   - CMake files are linted with `cmakelint`.
   - Ensure all files have the correct SPDX license headers (REUSE compliant).
6. **Commits**: We follow [Conventional Commits](https://www.conventionalcommits.org/) (validated by `commitizen`). This helps in automated changelog generation.
7. **Testing**: Build and run the tests inside the pixi environment:
   ```bash
   pixi run test
   ```
   During iteration, `pixi run build` rebuilds incrementally without re-running ctest. Use `pixi run geometry` to regenerate the geometry database and `pixi run gdml` to export GDML.
8. **Submission**: Open a Pull Request against the `main` branch. Ensure the CI passes.

## Licensing

This project is licensed under the **LGPL-3.0-or-later**. All contributions must be compatible with this license, and each new file must be covered by an SPDX identifier and copyright notice (see `REUSE.toml`).
