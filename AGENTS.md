# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `threshold-eval`
**Primary goal:** A C11 library that classifies a sampled `float` as OK, warning or trip against LOLO, LO, HI and HIHI limits that are validated once into a plan, with a policy-defined result for NaN and infinite samples, an epsilon dead-band, and caller-tracked hysteresis. No allocation, no system calls, no global state.

### 1.1 Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose
```

#### MISRA C:2023 (reads `build/compile_commands.json`)

```sh
env -u MISRA_RULE_TEXTS misch run --baseline
misch deviations
```

#### Formatting (pinned clang-format through `uvx`)

```sh
scripts/format.sh          # check, as CI does
scripts/format.sh --fix    # rewrite in place
```

### 1.2 Notes

- `meson setup` generates `threshold_eval_version.h` into the build directory; `threshold_eval.h` does not include it.
- The build produces only a static library, installed as `libthreshold_eval.a` with headers under `threshold_eval/` and `threshold_eval.pc`. The only Meson option is `build_tests`.
- `THRESHOLD_EVAL_MAX` and `THRESHOLD_EVAL_EPSILON` are read only in `src/threshold_eval.c`, so they take effect only as `-D` definitions for the library build. A definition before the include in a caller changes nothing.

---

## 2) Non-negotiable design invariants

- **Validate once.** Every configuration check lives in `threshold_plan_build()`. Evaluation trusts a plan whose `valid` flag is set and has no error path other than a non-finite sample.
- **Fail closed.** `valid` is cleared before any check, and a NULL or invalid plan evaluates to `THRESHOLD_SEV_INVALID`, never `OK`.
- **Finite before compare.** Every `float` input is checked with `isfinite` before it reaches a comparison or arithmetic (Directive 4.15). Non-finite samples resolve through the policy flags, in the order `FAILSAFE_TRIP`, `DEESCALATE_WARN`, `IGNORE_INVALID`.
- **Fixed comparison order.** `lolo`, `lo`, `hihi`, `hi`, each strict and widened by epsilon. A trip outranks the warning on its side.
- **Stateless.** Evaluation is a pure function of the plan and the sample. Hysteresis adjusts a copy of the plan; the caller owns `prev`.
- No heap, no system calls, no blocking, no global state.

---

## 3) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs (see §1.1 above).
- Install Python tools (Meson, Ninja, gcovr, misch) with `uv tool install`, never `pip`; CI does the same through `astral-sh/setup-uv`. CI installs `cppcheck` from Homebrew because Ubuntu's 2.13 reports different findings.
- `misch run --baseline` must pass against `analysis/baseline/`. Never regenerate the baseline to admit a new finding. Run it with `MISRA_RULE_TEXTS` unset, as CI does.
- The CI jobs are: tests under ASan/UBSan, a release build, a 100 percent line and branch coverage gate, a TI C2000 library build (CGT 22.6.2 and 25.11.1, COFF and EABI, warnings as errors), `clang-format`, and `misch`.

---

## 4) Docs / commit conventions

### Documentation layout

- `README.md` is a short landing page (under 100 lines). Put detail in `docs/`, never in the README; link to the owning file instead.
- `docs/integration.md` owns requirements, install methods, configuration, building, and cross builds.
- `docs/api.md` owns the API reference, the configuration macros and recipes. Change it together with the headers; keep signatures in fenced `c` blocks and describe them in prose, not function tables. Check every value an example states against a build.
- `docs/design.md` is the single authoritative design record: the plan model, validation, the comparison rules, non-finite handling, floating-point limits, failure behaviour, MISRA position, and verification.
- `CONTRIBUTING.md` documents the contributor workflow; `CHANGELOG.md` holds release history (Keep a Changelog); `.github/SECURITY.md` covers vulnerability reporting.
- Code blocks in `README.md`, `CONTRIBUTING.md`, and `docs/` are compiled by the `threshold_eval docs examples` test (`tests/docs/check_snippets.py`). Each document is compiled as one source file: a `c` fence is file-level code, `<!-- snippet: body -->` wraps statements, and `<!-- snippet: skip -->` excludes pseudo-code. Do not use `skip` to silence a broken example.
- Prose uses British spelling. Do not hard-wrap Markdown: write one line per paragraph. The 80-column limit applies to C source, not Markdown.

### Commits

- Use **Conventional Commits** format when asked to commit.
- Keep commits focused; explain *why* in the message body.

---

## 5) C style expectations

### Build & configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- Update `meson.build` and `tests/meson.build` when adding or removing files.
- `threshold_eval_conf.h` is public and installed beside `threshold_eval.h`. Every option in it stays behind an `#ifndef` guard.

### Formatting

- `.clang-format` is present and **mandatory**. Run `scripts/format.sh --fix` before committing. It runs the clang-format release pinned in the script through `uvx`. Do not use a system `clang-format`: another release can format the same code differently.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Style & correctness

- Match conventions in the existing files (indentation, braces, naming).
- Every public identifier uses the `threshold_` or `THRESHOLD_` prefix. Keep public headers minimal; do not expose internal helpers.
- Validate pointer arguments at every public API boundary; no `errno`.
- Use fixed-width types from `<stdint.h>`; never plain `int` for a fixed-width quantity.
- Enumerator values and the severity or status a given input produces are part of the API. Changing either is a breaking change.
- Public symbols carry Doxygen with the full contract: preconditions and return semantics.

### MISRA deviations

- Prefer compliant code; never restructure only to hide a finding. Deviate at the point of use with `/* cppcheck-suppress misra-c2012-<rule> ; @deviation <rationale> */`, or project-wide in `analysis/deviations/misra-deviations.txt` for a house-style rule.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on `enum`/`struct` members are allowed only when the resulting line fits the 80-column limit.
- If any member's annotation would overrun, move **all** of that aggregate's member docs into a single structured Doxygen block above the type, as an `@details` list of `- ::SYMBOL  description` entries.
- Never mix inline and block forms within one aggregate, and never leave a trailing comment that clang-format would wrap onto a second line.
- After editing, run `scripts/format.sh` and an 80-column scan.

### Testing

- Run `meson test -C build` after every change.
- Tests use the harness in `tests/test_harness.h` (`TEST_CASE`, `TEST_ASSERT`, `run_test`), one executable per concern. Do not add Unity or another external framework.
- Test each limit at the boundary, inside the dead-band, at its edge and past it.
- Add a test case for each bug fix and each new feature.
- Keep `src/` and `include/` at the coverage gate (100 percent line and branch).
