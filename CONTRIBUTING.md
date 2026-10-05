<!--
SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration

SPDX-License-Identifier: LGPL-3.0-or-later
-->

# Contributing to ship-random

Thank you for your interest in contributing. As part of the SHiP Collaboration, we follow a set of standards to keep the codebase clean and downstream-friendly.

## Development workflow

1. **Fork and clone** the repository.
2. **Environment**: the supported way to get the build dependencies (Random123, a recent CMake/Ninja/compiler) is [pixi](https://pixi.sh):
   ```bash
   pixi install
   pixi run test
   ```
   See `pixi.toml` for the full list of tasks (`configure`, `build`, `install`, `test`, `clean`).
3. **Pre-commit hooks**: we enforce style and licensing via [`prek`](https://github.com/j178/prek) (a drop-in `pre-commit` replacement). The hook tools come from the pixi `lint` environment, so versions are tracked in `pixi.lock` and run identically everywhere. Install the pre-commit and commit-msg hooks once:
   ```bash
   pixi run install-hooks
   ```
   Run all hooks manually at any time with `pixi run lint`.
4. **Branching**: create a feature branch for your changes.
5. **Coding standards**:
   - C++20; formatting enforced by `clang-format` (`.clang-format`).
   - C++ is checked against the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
     by `clang-tidy`, using the shared `.clang-tidy` synced from
     [ShipSoft/.github](https://github.com/ShipSoft/.github/tree/main/sync). Only
     findings on lines your patch changes are reported, so you are never asked to
     clean up code you did not touch. Whether those findings fail the build or are
     reported for information is set by `fail-on-new` in
     `.github/workflows/clang-tidy.yml`. The library is header-only, and CI
     reaches the headers through the tests that include them. To run the same
     analysis locally over the whole tree:
     ```bash
     pixi run clang-tidy
     ```
   - CMake formatting enforced by `gersemi`.
   - Every new file must carry an SPDX header (REUSE-compliant; verified by `reuse lint`).
6. **Commits**: we follow [Conventional Commits](https://www.conventionalcommits.org/), validated by [`committed`](https://github.com/crate-ci/committed). Allowed types are listed in `committed.toml` (`feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert`).
7. **Testing**: add tests for new behaviour. Run them with `pixi run test`.
   `tests/philox_rng_test.cpp` pins the numbers `PhiloxRng` draws. Every
   package that uses it puts those numbers into its physics output, so a change
   that makes this test fail changes simulated, digitised and reconstructed
   events. Such a change needs a new minor version and has to say so in the
   commit message.
8. **Submission**: open a pull request against `main`. CI (`Pixi Build` and `Lint`) must pass.

## Licensing

This project is licensed under **LGPL-3.0-or-later**. All contributions must be compatible with that licence; every file needs an SPDX identifier and copyright notice.
