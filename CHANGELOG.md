# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [1.3.0] - 2026-10-22

### Added

- CI cross-builds the library with TI C2000 CGT 22.6.2 and 25.11.1 under the COFF and EABI object formats, with warnings as errors.
- Validation tests covering every threshold field of every plan type for missing, out-of-range and extra values; `ALLOW_REORDER` tests for upper and lower plans; and hysteresis tests for an invalid plan and for a previous severity the plan type does not use.
- CI runs a MISRA C:2023 analysis with `misch` against a baseline in `analysis/baseline/` that records the existing findings, and checks formatting with the clang-format release pinned in `scripts/format.sh`.

### Changed

- `threshold_eval.h` defines `THRESHOLD_EVAL_VERSION_MAJOR`, `_MINOR`, `_PATCH`, `THRESHOLD_EVAL_VERSION`, `THRESHOLD_EVAL_VERSION_STRING` and a new `THRESHOLD_EVAL_VERSION_NUMBER` (`long`, `MAJOR * 10000 + MINOR * 100 + PATCH`), so a copied-in header carries its version. Meson fails to configure when the header and `project(version:)` disagree, and the build needs Meson 0.57 or later.
- CI gates line and branch coverage at 100 %, up from 80 % line and 70 % branch.
- CI installs Meson, Ninja and gcovr with `uv` instead of `pip`, and the Meson files and tests carry SPDX licence headers.
- The README is a short landing page. Installation, configuration and cross builds are in `docs/integration.md`, the API reference and recipes in `docs/api.md`, and the plan model, comparison rules, floating-point limits and verification in the new `docs/design.md`. A documentation test compiles every C example against the public headers.
- `CONTRIBUTING.md` and `AGENTS.md` describe the uv, clang-format and misch workflow and the validation and evaluation invariants.
- Checks that could not be reached are removed: the finiteness check inside the range check (callers check finiteness first), the argument guard of the internal sort, and the unknown-type branch of the second switch in `threshold_plan_build`. Results are unchanged.

### Removed

- The Sphinx and Doxygen site under `docs/sphinx/` and its CI job. `docs/api.md` is the API reference.
- `threshold_eval_version.h`. `threshold_eval.h` defines the version macros.
- The `extern "C"` guards in `threshold_eval.h`. The library is C11 only and its headers make no C++ claim.

## [1.2.0] - 2026-06-26

### Changed

- Added contributor guidance, security policy, SPDX headers, README badge correction, and a standard coverage gate.

### Fixed

- Removed a redundant plan-type range check in `threshold_plan_build`. Its lower bound (`type < THRESHOLD_TYPE_NONE`) is always false on targets whose `enum` representation is unsigned, provoking a "pointless comparison of unsigned integer with zero" warning, while its upper bound duplicated the dispatch `switch` default that already rejects out-of-domain types with `THRESHOLD_STATUS_INVALID_ARG`. Behaviour is unchanged for all inputs.

## [1.1.0] - 2026-04-04

### Added

- Configurable threshold evaluation with full-range, upper-only, lower-only, discrete warning, and discrete trip modes; validation plans; hysteresis; invalid-sample policies; epsilon dead-bands; strict configuration checks; helper functions; Meson packaging; CI; and unit tests.
- Sphinx/Breathe documentation generation from Doxygen annotations.
