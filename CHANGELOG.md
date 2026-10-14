# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- CI cross-builds the library with TI C2000 CGT 22.6.2 and 25.11.1 under the COFF and EABI object formats, with warnings as errors.
- Validation tests covering every threshold field of every plan type for missing, out-of-range and extra values; `ALLOW_REORDER` tests for upper and lower plans; and hysteresis tests for an invalid plan and for a previous severity the plan type does not use.

### Changed

- CI gates line and branch coverage at 100 %, up from 80 % line and 70 % branch.
- Checks that could not be reached are removed: the finiteness check inside the range check (callers check finiteness first), the argument guard of the internal sort, and the unknown-type branch of the second switch in `threshold_plan_build`. Results are unchanged.

## [1.2.0] - 2026-06-26

### Changed

- Added contributor guidance, security policy, SPDX headers, README badge correction, and a standard coverage gate.

### Fixed

- Removed a redundant plan-type range check in `threshold_plan_build`. Its lower bound (`type < THRESHOLD_TYPE_NONE`) is always false on targets whose `enum` representation is unsigned, provoking a "pointless comparison of unsigned integer with zero" warning, while its upper bound duplicated the dispatch `switch` default that already rejects out-of-domain types with `THRESHOLD_STATUS_INVALID_ARG`. Behaviour is unchanged for all inputs.

## [1.1.0] - 2026-04-04

### Added

- Configurable threshold evaluation with full-range, upper-only, lower-only, discrete warning, and discrete trip modes; validation plans; hysteresis; invalid-sample policies; epsilon dead-bands; strict configuration checks; helper functions; Meson packaging; CI; and unit tests.
- Sphinx/Breathe documentation generation from Doxygen annotations.
