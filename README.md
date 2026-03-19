# threshold-eval

Configurable threshold evaluation library for classifying sampled values against validated warning/trip limits with deterministic, policy-driven behavior.

## Features

- **Five evaluation types**: Full range (LOLO/LO/HI/HIHI), upper-only, lower-only, discrete warning, and discrete trip to match any monitoring pattern
- **Policy-driven invalid-sample handling**: Configurable per-plan response to NaN/Inf inputs — failsafe trip, de-escalate to warning, or ignore
- **Hysteresis support**: Prevents threshold chatter when a signal oscillates near a boundary, with caller-controlled state
- **Epsilon tolerance**: Configurable boundary dead-band protects against noise-induced false transitions
- **Threshold reordering**: Optional automatic sort of out-of-order thresholds at plan-build time
- **Strict configuration mode**: Rejects misconfigured plans (missing thresholds, wrong ordering, extra fields) at build time, not runtime
- **Stateless evaluation**: `threshold_plan_eval()` is a pure function with no hidden state making it safe to call from any context
- **Compliance-aware design goals**: Small auditable codebase with static allocation, explicit contracts, and unit-test coverage targeting IEC 61508 SIL 2 and MISRA C:2012

## Using the Library

### As a Meson subproject

```meson
threshold_eval_dep = dependency('threshold_eval', fallback: ['threshold_eval', 'threshold_eval_dep'])
```

The project also exports `meson.override_dependency('threshold_eval', ...)`
so downstream Meson builds can resolve the subproject dependency by name.

For subproject builds, include the public header directly:

```c
#include "threshold_eval.h"
```

To keep configuration separate from application code, you can optionally use
the public configuration header before including `threshold_eval.h`:

```c
#define THRESHOLD_EVAL_MAX 1000000.0f
#include "threshold_eval_conf.h"
#include "threshold_eval.h"
```

`threshold_eval.h` includes `threshold_eval_conf.h` automatically, so
defining `THRESHOLD_EVAL_MAX` or `THRESHOLD_EVAL_EPSILON` before
`threshold_eval.h` also works when you prefer a single include.

### As an installed dependency

If the library is installed system-wide, include the namespaced header path:

```c
#include <threshold_eval/threshold_eval.h>
```

The public configuration header is also installed as:

```c
#include <threshold_eval/threshold_eval_conf.h>
```

If `pkg-config` files are installed in your environment, downstream builds can
also discover the package as `threshold_eval`.

The generated version header is available as `threshold_eval_version.h` in the
build tree and as `<threshold_eval/threshold_eval_version.h>` after install.

## Building

```sh
# Library only (release)
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# With unit tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

## Quick Start

```c
#include "threshold_eval_conf.h"
#include "threshold_eval.h"

int main(void)
{
        threshold_config_t cfg;
        threshold_plan_t plan;

        /* Initialise config with safe defaults */
        threshold_config_init(&cfg);

        /* Configure a full-range voltage monitor (V) */
        cfg.type   = THRESHOLD_TYPE_RANGE;
        cfg.lolo   = 10.5f;   /* under-voltage trip    */
        cfg.lo     = 11.0f;   /* under-voltage warning */
        cfg.hi     = 14.5f;   /* over-voltage warning  */
        cfg.hihi   = 15.0f;   /* over-voltage trip     */
        cfg.policy = THRESHOLD_POLICY_FAILSAFE;

        /* Build and validate the plan once at startup */
        if (threshold_plan_build(&plan, &cfg) != THRESHOLD_STATUS_OK) {
                /* Handle configuration error */
                return 1;
        }

        /* Evaluate a sample at runtime (pure function, no side effects) */
        float voltage = read_voltage();
        threshold_severity_t sev = threshold_plan_eval(&plan, voltage);

        if (threshold_severity_is_trip(sev)) {
                trigger_protection();
        } else if (threshold_severity_is_warn(sev)) {
                raise_alarm();
        }

        return 0;
}
```

## API Reference

### Configuration Macros

| Macro | Description |
|-------|-------------|
| `THRESHOLD_EVAL_MAX` | Maximum absolute threshold value (configured via `threshold_eval_conf.h`, default: 4000000.0) |
| `THRESHOLD_EVAL_EPSILON` | Default boundary comparison epsilon (configured via `threshold_eval_conf.h`, default: 1e-3) |

### Public Configuration Header

```c
#define THRESHOLD_EVAL_MAX     1000000.0f
#define THRESHOLD_EVAL_EPSILON 0.01f
#include "threshold_eval_conf.h"
#include "threshold_eval.h"
```

### Type Definitions

| Type | Description |
|------|-------------|
| `threshold_type_t` | Plan type enum (NONE, RANGE, UPPER, LOWER, DISCRETE_WARN, DISCRETE_TRIP) |
| `threshold_policy_t` | Policy flag enum (FAILSAFE_TRIP, DEESCALATE_WARN, IGNORE_INVALID, STRICT_CONFIG, ALLOW_REORDER) |
| `threshold_severity_t` | Evaluation result enum (OK, WARN_LOW, WARN_HIGH, TRIP_LOW, TRIP_HIGH, INVALID) |
| `threshold_status_t` | Function return status enum |
| `threshold_config_t` | User-populated configuration structure |
| `threshold_plan_t` | Validated evaluation plan (output of `threshold_plan_build()`) |

### Composite Policy Macros

| Macro | Description |
|-------|-------------|
| `THRESHOLD_POLICY_FAILSAFE` | `FAILSAFE_TRIP \| STRICT_CONFIG` — recommended for safety-critical parameters |
| `THRESHOLD_POLICY_OPERATIONAL` | `DEESCALATE_WARN \| ALLOW_REORDER` — for informational parameters requiring continued operation |

### Core Functions

```c
threshold_status_t   threshold_config_init(threshold_config_t *config);
threshold_status_t   threshold_plan_build(threshold_plan_t *plan,
                                          const threshold_config_t *config);
threshold_severity_t threshold_plan_eval(const threshold_plan_t *plan,
                                         float sample);
threshold_severity_t threshold_plan_eval_hys(const threshold_plan_t *plan,
                                             float sample, float hysteresis,
                                             threshold_severity_t prev);
```

### Severity Classification Helpers

```c
static inline bool threshold_severity_is_warn(threshold_severity_t sev);
static inline bool threshold_severity_is_trip(threshold_severity_t sev);
static inline bool threshold_severity_is_low(threshold_severity_t sev);
static inline bool threshold_severity_is_high(threshold_severity_t sev);
```

### Diagnostic String Functions

```c
const char *threshold_severity_name(threshold_severity_t sev);
const char *threshold_type_name(threshold_type_t type);
const char *threshold_status_str(threshold_status_t status);
```

## Use Cases

- **Process variable monitoring**: Classify a sensor reading against LOLO/LO/HI/HIHI band limits
  ```c
  cfg.type = THRESHOLD_TYPE_RANGE;
  cfg.lolo = 0.5f; cfg.lo = 1.0f; cfg.hi = 4.0f; cfg.hihi = 4.5f;
  threshold_plan_build(&plan, &cfg);
  threshold_severity_t sev = threshold_plan_eval(&plan, pressure);
  ```

- **Over-temperature protection**: Monitor only the high side with a two-level trip chain
  ```c
  cfg.type = THRESHOLD_TYPE_UPPER;
  cfg.hi = 85.0f; cfg.hihi = 95.0f;
  threshold_plan_build(&plan, &cfg);
  ```

- **Sensor failure handling**: Route NaN/Inf inputs to a deterministic protective action
  ```c
  cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
  /* NaN sample -> THRESHOLD_SEV_TRIP_HIGH, drives the same path as a real trip */
  ```

- **Chatter prevention with hysteresis**: Prevent rapid warn/ok/warn transitions near a boundary
  ```c
  static threshold_severity_t prev = THRESHOLD_SEV_OK;
  threshold_severity_t current =
      threshold_plan_eval_hys(&plan, sample, 2.0f, prev);
  prev = current;
  ```

- **Discrete digital threshold**: Single-threshold warning for a computed ratio or percentage
  ```c
  cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
  cfg.hi = 80.0f; /* warn if imbalance > 80% */
  threshold_plan_build(&plan, &cfg);
  ```

## Notes

| Topic | Note |
|-------|-------|
| **Threshold range** | All thresholds must satisfy `\|value\| <= THRESHOLD_EVAL_MAX` (default 4,000,000) |
| **Monotonic order** | LOLO <= LO <= HI <= HIHI; enforced by STRICT_CONFIG or corrected by ALLOW_REORDER |
| **Epsilon** | Applied as dead-band at boundaries; set to 0.0f to disable; per-plan via `threshold_config_t.epsilon` |
| **Non-finite samples** | NaN and Inf are detected before comparison; result driven entirely by policy flags |
| **Hysteresis state** | The caller must track `prev` between calls; `threshold_plan_eval_hys()` is stateless |
| **Thread safety** | `threshold_plan_eval()` is a pure function and is safe for concurrent callers sharing a read-only plan; `threshold_plan_build()` must not be called concurrently on the same plan |
| **Plan validity** | A plan must not be used for evaluation unless `plan.valid == true`; always check the return of `threshold_plan_build()` |
| **No dynamic memory** | No heap allocation; all structures are caller-owned and can be stack- or statically-allocated |
