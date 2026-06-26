# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [1.2.0] - 2026-06-26

### Changed

- Added contributor guidance, security policy, SPDX headers, README badge correction, and a standard coverage gate.

### Fixed

- Removed a redundant plan-type range check in `threshold_plan_build`. Its lower bound (`type < THRESHOLD_TYPE_NONE`) is always false on targets whose `enum` representation is unsigned, provoking a "pointless comparison of unsigned integer with zero" warning, while its upper bound duplicated the dispatch `switch` default that already rejects out-of-domain types with `THRESHOLD_STATUS_INVALID_ARG`. Behaviour is unchanged for all inputs.

## [1.1.0] - 2026-04-04

### Added

- Configurable threshold evaluation with full-range, upper-only, lower-only, discrete warning, and discrete trip modes; validation plans; hysteresis; invalid-sample policies; epsilon dead-bands; strict configuration checks; helper functions; Meson packaging; CI; and unit tests.
- Sphinx/Breathe documentation generation from Doxygen annotations.
