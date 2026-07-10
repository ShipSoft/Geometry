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
   - Follow the existing C++ style (enforced by `clang-format` and `cpplint`).
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
