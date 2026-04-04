# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.1.0] - 2026-04-04

### Added

- Five evaluation types: full range (LOLO/LO/HI/HIHI), upper-only,
  lower-only, discrete warning, and discrete trip
- Two-phase API: `threshold_plan_build()` validates once,
  `threshold_plan_eval()` evaluates repeatedly with no allocation
- Policy-driven invalid-sample handling for NaN/Inf inputs with three
  configurable responses: failsafe trip, de-escalate to warning, or ignore
- Composite policy macros `THRESHOLD_POLICY_FAILSAFE` and
  `THRESHOLD_POLICY_OPERATIONAL` for common configurations
- Hysteresis support via `threshold_plan_eval_hys()` to prevent threshold
  chatter near boundaries with caller-controlled state
- Configurable epsilon dead-band at each threshold boundary to suppress
  noise-induced false transitions
- Optional automatic threshold reordering at plan-build time via
  `THRESHOLD_POLICY_ALLOW_REORDER`
- Strict configuration mode (`THRESHOLD_POLICY_STRICT_CONFIG`) that rejects
  extra thresholds, missing thresholds, and ordering violations at build time
- Severity classification helpers: `threshold_severity_is_warn()`,
  `threshold_severity_is_trip()`, `threshold_severity_is_low()`,
  `threshold_severity_is_high()`
- Diagnostic string functions: `threshold_severity_name()`,
  `threshold_type_name()`, `threshold_status_str()`
- User-overridable configuration macros `THRESHOLD_EVAL_MAX` and
  `THRESHOLD_EVAL_EPSILON` via `threshold_eval_conf.h`
- Meson build system with subproject support, pkg-config generation, and
  optional unit test builds
- CI pipeline (GitHub Actions) for Ubuntu and macOS with address and
  undefined-behaviour sanitisers
- Unit test suite covering configuration, validation, evaluation, hysteresis,
  policy resolution, boundary dead-bands, threshold ordering, and helper
  functions
