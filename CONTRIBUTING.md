# Contributing to threshold-eval

`threshold-eval` is a small C11 library that classifies sampled values against validated warning and trip limits. A monitor that reports `OK` when it should trip is the failure that matters, so read [design.md](docs/design.md) before you change validation, the comparison order or the non-finite handling.

## Getting started

Install a C11 compiler and install the Python tools with [uv](https://docs.astral.sh/uv/), as CI does. The MISRA analysis also needs a current `cppcheck`; CI installs it from Homebrew because the 2.13 release in Ubuntu 24.04 reports different findings. The same commands CI runs, locally:

```sh
# Python tools
uv tool install meson
uv tool install ninja
uv tool install gcovr
uv tool install misch

# Configure with tests and sanitisers (CI default)
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# Release build (the configuration a downstream consumer uses)
meson setup build-rel --buildtype=release -Dbuild_tests=false
meson compile -C build-rel

# Coverage (CI gate is 100% line and branch for src/ and include/)
meson setup build-cov --buildtype=debug -Dbuild_tests=true -Db_coverage=true
meson compile -C build-cov && meson test -C build-cov
gcovr --root . --filter 'src/' --filter 'include/' --fail-under-line 100 --fail-under-branch 100 --print-summary

# MISRA C:2023 (reads build/compile_commands.json)
env -u MISRA_RULE_TEXTS misch run --baseline
misch deviations

# Formatting (pinned clang-format through uvx)
scripts/format.sh --fix
```

## Source style

- Formatting is mandatory. Run `scripts/format.sh --fix` before submitting, or `scripts/format.sh` to check without writing, as CI does. The script pins the clang-format release and runs it through `uvx`, so an editor formatter gives the same result only when it runs the same release.
- 8-space indent, Linux brace style, 80-column limit. Match the existing conventions and do not reformat unrelated code.
- The Meson build system is the single source of truth. Update `meson.build` and `tests/meson.build` when you add or remove files. No CMake, no Make, and no other build systems.

## C language rules

- C11 only.
- Use fixed-width types from `<stdint.h>` and `<stdbool.h>`, and never plain `int` for a fixed-width quantity.
- No heap allocation (`malloc`, `free`, VLAs), no system calls, and no global state.
- Validate every pointer argument at the public API boundary, and return a status or `THRESHOLD_SEV_INVALID` rather than fault.
- Check every `float` that enters the library for finiteness before it reaches a comparison or an arithmetic operation.
- Every public identifier uses the `threshold_` or `THRESHOLD_` prefix.

## Validation and evaluation rules

- Every check on a configuration belongs in `threshold_plan_build()`. Evaluation trusts a valid plan and has no error path other than a non-finite sample.
- Evaluation stays a pure function of the plan and the sample. Hysteresis state belongs to the caller.
- A new plan type needs its own validation, its own evaluation, and a row in each type table in `docs/api.md`.
- Do not reorder the comparisons, change which severity a policy flag produces, or relax a strict check without an issue that agrees the change first. Each changes what an existing monitor reports.

## MISRA C:2023

The library is analysed with `misch` (cppcheck-backed MISRA C:2023 analysis), configured by `misra.toml`, over `include/` and `src/`. CI fails on any finding that is not in the baseline in `analysis/baseline/`. Run it from a configured `build/` directory, with `MISRA_RULE_TEXTS` unset, as CI does. If your change introduces a finding:

1. Prefer restructuring the code so the rule is satisfied. Deviate only when compliance would make the code genuinely worse, and never restructure purely to hide a finding from the checker; an honest deviation beats evasion.
2. Suppress at the point of use with `/* cppcheck-suppress misra-c2012-<rule> ; @deviation <rationale> */`, or add a justified project-wide entry to `analysis/deviations/misra-deviations.txt` for a house-style rule.
3. Verify with `misch run --baseline` and `misch deviations`, which requires every suppression to carry a rationale, and justify the deviation in the pull request description.

Do not regenerate the baseline to admit a new finding. It changes only after a deliberate review, with `misch baseline`.

## Tests and coverage

- Add a test for every bug fix and every new feature.
- Tests live in `tests/test_*.c`, one executable per concern, written with the `TEST_CASE`, `TEST_ASSERT` and `run_test` macros from `tests/test_harness.h`. Do not add Unity or another external test framework. Each new file is listed in `tests/meson.build`.
- Test each limit at the boundary, inside the dead-band, at its edge and past it. A test that only samples the middle of a band does not detect a comparison that changed from `<` to `<=`.
- CI enforces 100 percent line and branch coverage of `src/` and `include/`. Every new branch needs a test that fails when the branch is broken.
- The `threshold_eval docs examples` test compiles every C code block in `README.md`, `CONTRIBUTING.md`, and `docs/` against the public headers, through `tests/docs/check_snippets.py`. A block compiles at file scope by default. On the line directly above the fence, put `<!-- snippet: body -->` for a block of statements, or `<!-- snippet: skip -->` for pseudo-code or a copy of a header definition.

## Documentation

| File | Purpose |
| --- | --- |
| `README.md` | Landing page: quick start, description, validated toolchains, and links. Keep it short. |
| `docs/integration.md` | Requirements, installation, configuration, building, and cross builds. |
| `docs/api.md` | Reference for configuration, plan building, evaluation, hysteresis, helpers, diagnostics, the configuration macros, and recipes. |
| `docs/design.md` | The plan model, validation, the comparison rules, non-finite handling, floating-point limits, failure behaviour, MISRA position, and verification. |

Update `docs/api.md` and the header Doxygen together when an interface changes, and check every value an example states against a build. Update `docs/design.md` when validation, evaluation or the verification changes. Keep it standalone: describe how the library works, not how it was built or which other projects use it. Write prose in British English, one line per paragraph.

## API stability

- The public API in `threshold_eval.h` and the configuration macros in `threshold_eval_conf.h` are stable within a minor release line.
- The numeric values of `threshold_severity_t`, `threshold_status_t`, `threshold_type_t` and the policy flags are part of the contract, because callers store and transmit them.
- Changing a function signature, an enumerator value, the severity a given input produces, or the status a given configuration returns is a breaking change and needs a major release.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `docs: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` a change that neither fixes a bug nor adds a feature

Keep the subject under about 70 characters. Use the body to explain why the change is needed.

## Pull requests

- Open an issue first for non-trivial changes, and always for a change to validation or evaluation results, so the design can be agreed before implementation.
- Keep pull requests focused: one feature or one fix.
- All CI checks must pass.
- Add a `CHANGELOG.md` entry under `## [Unreleased]` for a user-visible change.
- Flag any new MISRA deviation, and any change to what a configuration or sample produces, in the description.

## When in doubt

Open an issue. The library is small, but a change to one comparison changes what every monitor built on it reports.
