# AGENTS.md

## Project-specific instructions

**Project:** `threshold-eval`
**Primary goal:** Configurable threshold evaluation library for classifying sampled values against validated warning/trip limits with deterministic, policy-driven behavior. Targets IEC 61508 SIL 2 and MISRA C:2012 compliance.

### Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

### CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs.
- If `pre-commit` is configured later, run it before committing.

## Docs / commit conventions

- Use Conventional Commits when asked to commit.
- Keep commits focused and explain why the change exists.

## C style expectations

### Build and configuration

- Use the Meson build system; do not introduce another build system.
- Update `meson.build` when adding or removing source files or public headers.
- `threshold_eval_conf.h` is a public configuration header; install it alongside `threshold_eval.h`.

### Formatting

- `.clang-format` is present and should be used on modified `.c` and `.h` files.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Style and correctness

- Match the conventions in the existing files.
- All public identifiers use the `threshold_` / `THRESHOLD_` prefix.
- Keep public headers minimal and stable; do not expose internal helpers.
- Prefer explicit fixed-width integer types when ABI or serialization matters.
- All floating-point inputs entering the library must be validated for finiteness (MISRA C:2012 Dir 4.15).
- No dynamic memory allocation anywhere in the library.

### API design

- Configuration and validation are separated: populate `threshold_config_t`, validate once with `threshold_plan_build()`, evaluate repeatedly with `threshold_plan_eval()`.
- Evaluation functions are pure/stateless: no side effects, no hidden global state.
- Hysteresis state is owned by the caller, not the library.
- New evaluation types must go through `threshold_plan_build()` validation before use.

### Testing

- Run `meson test -C build` after changes.
- Tests live in `tests/test_threshold_eval.c` using the lightweight built-in test framework (no external test dependencies).
- Add a test case for each bug fix and each new feature.
- Do not add Unity or other test framework dependencies; use the existing `TEST_CASE` / `TEST_ASSERT` / `run_test` pattern.
