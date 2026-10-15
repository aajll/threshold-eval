# Design

`threshold-eval` turns a sampled value into a severity, against limits that were checked once before the first sample arrived. This document describes the plan model, how a configuration is validated, how a sample is classified, where single-precision arithmetic limits the result, and how the library is verified. The API is in [api.md](api.md).

## Goals

- Every configuration error is found when the plan is built, so evaluation has no error path for a valid plan and a finite sample.
- A failed sensor gives a defined, configured result rather than whatever a NaN comparison happens to produce.
- Evaluation is a pure function of the plan and the sample: no allocation, no global state, constant time.
- Portable C11 with single-precision `float`, including targets where `CHAR_BIT` is 16.

Filtering, debouncing, latching, rate-of-change limits and timing are out of scope. A project that needs them builds them around the severity.

## Plans

A configuration (`threshold_config_t`) is what the caller writes; a plan (`threshold_plan_t`) is what the library has checked. The plan holds the same four limits, the epsilon, the type and the policy, plus a `valid` flag that only a successful `threshold_plan_build` sets. Evaluation reads only the plan, so the configuration can be discarded or edited after the build without affecting a running monitor.

A limit is "not set" when it is NaN. `threshold_config_init` writes NaN to all four, so a configuration that names only the limits its type uses carries no stray zeros, and a strict build can tell a forgotten limit from a deliberate one. The plan stores NaN for every limit its type does not use.

## Validation

`threshold_plan_build` clears `valid` before anything else, then checks, in order: the pointers, the epsilon, the type, that each limit the type uses is finite, that each is within `THRESHOLD_EVAL_MAX` of zero, and, under `STRICT_CONFIG`, that the unused limits are unset and the used ones ascend. It returns the first failure, and `valid` stays false. [api.md](api.md#building-a-plan) lists the status for each check.

`STRICT_CONFIG` exists because a monitor with a typo in its limits is worse than no monitor: it reports `OK` for a condition it was meant to catch. A finite value in a limit the type ignores is most often a type set wrongly, so a strict build rejects it rather than silently dropping it.

`ALLOW_REORDER` serves the opposite case, limits from a source that does not guarantee their order, such as an operator panel. It sorts the limits the type uses, by an insertion sort of at most four values. For a `RANGE` plan it also turns off the strict order check, so a strict, reordering configuration is sorted. For `UPPER` and `LOWER` the strict order check runs before the swap, so the same policy rejects an inverted pair with `THRESHOLD_STATUS_ORDER`.

`THRESHOLD_EVAL_MAX` bounds the limits, not the samples. Its default of 4,000,000 keeps every limit where the spacing between adjacent `float` values is at most 0.25.

## Evaluation

A valid plan and a finite sample go through at most four comparisons, low side first:

1. `sample < lolo - epsilon`: `TRIP_LOW`
2. `sample < lo - epsilon`: `WARN_LOW`
3. `sample > hihi + epsilon`: `TRIP_HIGH`
4. `sample > hi + epsilon`: `WARN_HIGH`

Each type runs only the comparisons for the limits it uses. Because a trip is tested before the warning on its side, a sample past both reports the trip. The comparisons are strict, so a sample exactly at a limit is not past it, and `epsilon` widens that boundary into a dead-band.

The order of the comparisons matters only for limits that are out of order. A non-strict `RANGE` plan with `hi` of 90 and `hihi` of 80 builds, and a sample of 85 reports `TRIP_HIGH`, because it is above `hihi` and that comparison comes first. A `lolo` above the other limits makes every sample below it a `TRIP_LOW`. Use `STRICT_CONFIG` or `ALLOW_REORDER` unless the limits are known to ascend.

## Non-finite samples

A NaN compares false with everything, so without a check a NaN sample would pass all four comparisons and report `OK`, and an infinite one would report a trip on one side. Evaluation therefore tests `isfinite` before comparing and hands a NaN or infinite sample to the policy:

| Policy flag | Result |
| --- | --- |
| `FAILSAFE_TRIP` | `TRIP_HIGH` |
| `DEESCALATE_WARN` | `WARN_HIGH` |
| `IGNORE_INVALID` | `OK` |
| none of these | `INVALID` |

The flags are tested in that order, so the most protective flag present wins. The failsafe and de-escalate results are on the high side for every plan type, including `LOWER`: they mean "act as if this limit tripped", not "the value is high". A caller that needs to tell a lost sensor from a real excursion uses no non-finite flag and handles `INVALID` itself.

A `NONE` plan returns `OK` before the sample is examined, so a disabled monitor stays quiet even when its input is NaN.

## Hysteresis

`threshold_plan_eval_hys` copies the plan, moves the one limit that `prev` names inwards by `hysteresis`, and evaluates the copy. The plan itself is never modified, and the caller holds the previous severity, so the library stays stateless and one plan can serve several signals.

Moving only the limit that was crossed keeps the adjustment local. A signal in `TRIP_HIGH` returns to `WARN_HIGH` once it falls below `hihi - hysteresis`; on the next call `prev` is `WARN_HIGH`, and the margin applies to `hi`. Hysteresis larger than the gap between two limits moves a limit past its neighbour for that call only, and the fixed comparison order then decides the result.

## Floating-point limits

Limits, samples, epsilon and hysteresis are all `float`, and each comparison adds or subtracts in `float`. An epsilon smaller than half the spacing between `float` values at a limit rounds away: `limit + epsilon` equals `limit`, and the dead-band is gone. The spacing is 2^-13 between 1024 and 2048 and doubles at every power of two, reaching 2^-8 (about 0.0039) at 32768, so the default epsilon of 1e-3 has no effect for limits of magnitude 32768 and above. Scale the epsilon with the limits, or keep limits in units that make them small.

The same rounding applies to hysteresis: a margin below half the spacing at the adjusted limit does not move it.

Epsilon and hysteresis must be finite and non-negative. A build rejects a bad epsilon with `THRESHOLD_STATUS_INVALID_ARG`, and `threshold_plan_eval_hys` returns `INVALID` for a bad hysteresis, because a negative margin would move a limit outwards.

## Failure behaviour

| Condition | Result |
| --- | --- |
| NULL configuration or plan in a build | `THRESHOLD_STATUS_INVALID_ARG`; a non-NULL plan is marked invalid. |
| Any build failure | `valid` is false; the plan's other members are unspecified. |
| NULL or invalid plan in an evaluation | `INVALID`. |
| NaN or infinite sample | Resolved by the policy flags. |
| NaN, infinite or negative hysteresis | `INVALID`. |
| Severity, type or status outside its enumeration | The name functions return `"UNKNOWN"` or `"Unknown status"`. |
| Limits out of order without `STRICT_CONFIG` or `ALLOW_REORDER` | Built and compared as given. |
| Epsilon below the `float` spacing at a limit | No dead-band at that limit. |

Policy bits outside the five defined flags are stored and ignored.

## MISRA C:2023

The library is analysed with [`misch`](https://pypi.org/project/misch/), a cppcheck-based MISRA C:2023 checker, over `include/` and `src/`. The baseline of accepted findings is in `analysis/baseline/`, and CI fails on any finding outside it. It holds 107 findings:

| Rule | Findings | Cause |
| --- | --- | --- |
| 15.5 | 76 | Early returns, which reject invalid input or return a result as soon as it is known. |
| 10.4 | 10 | A `uint32_t` policy masked with a `threshold_policy_t` enumerator. |
| 12.1 | 10 | `limit - eps` and `limit + eps` as comparison operands without parentheses. |
| 8.7 | 7 | Public functions with no caller outside `src/threshold_eval.c` in the analysed code. |
| 10.8 | 1 | The cast in `THRESHOLD_POLICY_FAILSAFE`, used by `threshold_config_init`. |
| 2.3, 2.4 | 1 each | `threshold_policy_t` is never used as a type, because the policy is stored as `uint32_t`. |
| 2.5 | 1 | `THRESHOLD_POLICY_OPERATIONAL`, which the library defines for its callers. |

The non-finite checks before every comparison follow Directive 4.15, which asks that NaN and infinite values be handled rather than propagated. The baseline records the findings as they stand. Formal compliance needs a certified analyser and a deviation record from the consuming project.

## Verification

The tests run on the host, each registered as `threshold_eval_<name>`:

| Test | What it checks |
| --- | --- |
| `config` | The defaults that `threshold_config_init` sets, and NULL handling. |
| `validation`, `ordering` | Every build status for every type: each missing, out-of-range and extra field, epsilon validation, equal limits, each order violation, and reordering. |
| `evaluation`, `boundaries` | Each type's severities, and samples at, inside and past each dead-band with zero, default and custom epsilon, including negative limits. |
| `policies`, `nonfinite_samples` | NaN and infinite samples under each policy flag and plan type, flag priority, and the composite policies. |
| `hysteresis` | Hold and release on both sides for each type, consecutive evaluations, `prev` values a type does not use, and invalid hysteresis. |
| `helpers` | The severity helpers and the name functions, including values outside each enumeration. |
| `edge_cases` | Unknown types, hysteresis wider than a gap, `NONE` plans with non-finite samples, rebuilding after a failure, and extreme epsilon. |
| `threshold_eval docs examples` | Every C block in the README, `CONTRIBUTING.md` and `docs/` compiles against the public headers. |

CI runs every test under AddressSanitizer and UndefinedBehaviorSanitizer, gates line and branch coverage of `src/` and `include/` at 100 percent, builds the library in release mode, checks formatting, and runs the MISRA analysis. It also compiles the library with warnings as errors under TI C2000 CGT 22.6.2 and 25.11.1, in both the COFF and EABI formats.
