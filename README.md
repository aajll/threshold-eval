# threshold-eval

[![CI](https://github.com/aajll/threshold-eval/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/threshold-eval/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**A C11 library that classifies a sampled value as OK, warning or trip against validated LOLO, LO, HI and HIHI limits, with a configured response to NaN and infinite samples.**

## Quick start

Copy `include/threshold_eval.h`, `include/threshold_eval_conf.h` and `src/threshold_eval.c` into your project. For a Meson subproject or an installed `pkg-config` package, see the [integration guide](docs/integration.md).

```c
#include <stdio.h>

int
main(void)
{
        threshold_config_t cfg;
        threshold_plan_t plan;

        /* Unused thresholds NaN, failsafe policy, default epsilon. */
        (void)threshold_config_init(&cfg);

        /* A 12 V supply monitor. */
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.5f; /* under-voltage trip */
        cfg.lo = 11.0f;   /* under-voltage warning */
        cfg.hi = 14.5f;   /* over-voltage warning */
        cfg.hihi = 15.0f; /* over-voltage trip */

        /* Validate once, at start-up. */
        if (threshold_plan_build(&plan, &cfg) != THRESHOLD_STATUS_OK) {
                return 1;
        }

        /* Prints OK, WARN_HIGH, TRIP_HIGH, TRIP_LOW. */
        const float samples[] = {12.6f, 14.8f, 15.2f, 10.0f};
        for (unsigned i = 0u; i < 4u; ++i) {
                threshold_severity_t sev = threshold_plan_eval(&plan, samples[i]);
                printf("%s\n", threshold_severity_name(sev));
        }
        return 0;
}
```

## Description

A plan is built once from a configuration and then evaluated against each sample. Building validates every limit, so evaluation has no error path other than a non-finite sample.

- Five plan types: full range (LOLO, LO, HI, HIHI), upper only, lower only, a single warning limit, and a single trip limit
- Policy flags choose what a NaN or infinite sample produces (trip, warning, OK or `INVALID`) and how strictly a configuration is checked
- An epsilon dead-band at each limit and caller-tracked hysteresis stop a noisy signal flickering between levels
- Evaluation is a pure function of the plan and the sample: no state, no allocation, safe from any context that can read the plan
- Tests under ASan and UBSan with 100% line and branch coverage, and MISRA C:2023 analysis with [`misch`](https://pypi.org/project/misch/)

## Validated toolchains

| Toolchain               | Target                        | Validation                          |
| ----------------------- | ----------------------------- | ----------------------------------- |
| GCC                     | x86_64 Linux                  | All tests under ASan and UBSan      |
| DSP vendor C11 compiler | 16-bit MAU (`CHAR_BIT == 16`) | Library compile, two object formats |

Target linking and execution belong to the consuming firmware. See [cross builds](docs/integration.md#cross-builds).

## Documentation

- [Integration guide](docs/integration.md) — copy-in, Meson subproject, installed package, configuration, building, and cross builds
- [API reference](docs/api.md) — configuration, plan building, evaluation, hysteresis, helpers, and recipes
- [Design](docs/design.md) — the plan model, validation, the comparison rules, floating-point limits, failure behaviour, and verification
- [Changelog](CHANGELOG.md) — release history

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request. Report security issues as described in [SECURITY.md](.github/SECURITY.md).

## License

MIT. See [LICENSE](LICENSE).
