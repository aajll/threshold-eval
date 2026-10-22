# API reference

`threshold_eval.h` declares the whole interface and includes `threshold_eval_conf.h`. The examples in this document compile together as one file after `#include "threshold_eval.h"`.

## Conventions

- Every structure is caller-owned and may be static, on the stack, or embedded in another structure. No function allocates, blocks or keeps state between calls.
- A plan is built once, typically at start-up, and is read-only afterwards. Any number of contexts may evaluate the same plan at the same time; rebuilding it while another context evaluates it is a data race.
- Every function checks its pointers. A NULL argument never dereferences.

## Configuration

```c
static threshold_config_t motor_temp_cfg;

void
motor_temp_configure(void)
{
        (void)threshold_config_init(&motor_temp_cfg);
        motor_temp_cfg.type = THRESHOLD_TYPE_UPPER;
        motor_temp_cfg.hi = 85.0f;   /* warn above 85 degC */
        motor_temp_cfg.hihi = 95.0f; /* trip above 95 degC */
}
```

```c
threshold_status_t threshold_config_init(threshold_config_t *config);
```

`threshold_config_init` sets all four thresholds to NaN, `epsilon` to `THRESHOLD_EVAL_EPSILON`, `type` to `THRESHOLD_TYPE_NONE` and `policy` to `THRESHOLD_POLICY_FAILSAFE`. It returns `THRESHOLD_STATUS_INVALID_ARG` if `config` is NULL, and `THRESHOLD_STATUS_OK` otherwise. Start every configuration from it: a NaN threshold reads as "not set", which is how plan building tells a missing limit from a limit of zero.

<!-- snippet: skip -->
```c
typedef struct {
        float lolo;
        float lo;
        float hi;
        float hihi;
        float epsilon;
        threshold_type_t type;
        uint32_t policy;
} threshold_config_t;
```

`lolo` and `lo` are the low trip and low warning limits, `hi` and `hihi` the high warning and high trip limits. `epsilon` widens the dead-band at each limit and must be finite and at least zero. `policy` is a bitwise OR of the flags below.

### Plan types

| Type | Limits used | Results |
| --- | --- | --- |
| `THRESHOLD_TYPE_NONE` | none | Always `OK`. |
| `THRESHOLD_TYPE_RANGE` | `lolo`, `lo`, `hi`, `hihi` | Any severity. |
| `THRESHOLD_TYPE_UPPER` | `hi`, `hihi` | `OK`, `WARN_HIGH`, `TRIP_HIGH`. |
| `THRESHOLD_TYPE_LOWER` | `lolo`, `lo` | `OK`, `WARN_LOW`, `TRIP_LOW`. |
| `THRESHOLD_TYPE_DISCRETE_WARN` | `hi` | `OK`, `WARN_HIGH`. |
| `THRESHOLD_TYPE_DISCRETE_TRIP` | `hihi` | `OK`, `TRIP_HIGH`. |

### Policy flags

| Flag | Effect |
| --- | --- |
| `THRESHOLD_POLICY_FAILSAFE_TRIP` | A NaN or infinite sample evaluates to `TRIP_HIGH`. |
| `THRESHOLD_POLICY_DEESCALATE_WARN` | A NaN or infinite sample evaluates to `WARN_HIGH`. |
| `THRESHOLD_POLICY_IGNORE_INVALID` | A NaN or infinite sample evaluates to `OK`. |
| `THRESHOLD_POLICY_STRICT_CONFIG` | Plan building rejects limits the type does not use, and limits out of order. |
| `THRESHOLD_POLICY_ALLOW_REORDER` | Plan building sorts the limits into ascending order. |

With none of the first three flags set, a non-finite sample evaluates to `INVALID`. If more than one is set, the first in the table wins, so a failsafe flag cannot be weakened by adding another. `THRESHOLD_POLICY_NONE` is zero.

Two composites cover the common cases. `THRESHOLD_POLICY_FAILSAFE` is `FAILSAFE_TRIP | STRICT_CONFIG`, for a protective input whose loss must stop the process; it is the default. `THRESHOLD_POLICY_OPERATIONAL` is `DEESCALATE_WARN | ALLOW_REORDER`, for an informational input whose loss should be reported without stopping anything.

## Building a plan

```c
static threshold_plan_t motor_temp_plan;

bool
motor_temp_start(void)
{
        threshold_status_t status =
            threshold_plan_build(&motor_temp_plan, &motor_temp_cfg);
        return status == THRESHOLD_STATUS_OK;
}
```

```c
threshold_status_t threshold_plan_build(threshold_plan_t *plan,
                                        const threshold_config_t *config);
```

`threshold_plan_build` validates `config` and, if it is usable, writes a plan and sets its `valid` member. It never modifies `config`. On failure `plan->valid` is `false` and the other plan members are unspecified; an invalid plan evaluates to `INVALID`, so a plan whose build failed cannot be mistaken for one reporting `OK`.

The checks run in this order, and the first failure is returned:

1. `plan` or `config` NULL, or `epsilon` NaN, infinite or negative: `THRESHOLD_STATUS_INVALID_ARG`.
2. `type` is `NONE`: the plan is valid, with every limit NaN.
3. `type` outside the enumeration: `THRESHOLD_STATUS_INVALID_ARG`.
4. A limit the type uses is NaN or infinite: `THRESHOLD_STATUS_MISSING`.
5. A limit the type uses has a magnitude above `THRESHOLD_EVAL_MAX`: `THRESHOLD_STATUS_OUT_OF_RANGE`.
6. Under `STRICT_CONFIG`, a limit the type does not use is finite: `THRESHOLD_STATUS_EXTRA`. A `RANGE` plan uses every limit, so it never fails this check.
7. Under `STRICT_CONFIG`, the limits used are not in ascending order (`lolo <= lo <= hi <= hihi`): `THRESHOLD_STATUS_ORDER`.

`ALLOW_REORDER` sorts the limits a type uses into ascending order before they are stored. For a `RANGE` plan it also suppresses the order check, so `STRICT_CONFIG | ALLOW_REORDER` sorts. For `UPPER` and `LOWER` the order check runs first, so the same combination returns `THRESHOLD_STATUS_ORDER` for an inverted pair.

Without `STRICT_CONFIG` or `ALLOW_REORDER`, out-of-order limits build a valid plan and are compared as given; [design.md](design.md#evaluation) shows what that produces.

The plan copies `type`, `policy` and `epsilon`. Limits the type does not use are NaN in the plan, whatever the configuration held.

## Evaluating a sample

```c
void
motor_temp_poll(float temp_c)
{
        threshold_severity_t sev = threshold_plan_eval(&motor_temp_plan, temp_c);

        if (threshold_severity_is_trip(sev)) {
                /* open the contactor */
        } else if (threshold_severity_is_warn(sev)) {
                /* raise an operator alarm */
        }
}
```

```c
threshold_severity_t threshold_plan_eval(const threshold_plan_t *plan,
                                         float sample);
```

`threshold_plan_eval` classifies `sample` against the plan. It returns `INVALID` if `plan` is NULL or not valid, and `OK` for a `NONE` plan, before it looks at the sample. A NaN or infinite sample then resolves through the policy flags. A finite sample is compared with each limit the type uses, widened by `epsilon`:

| Condition | Result |
| --- | --- |
| `sample < lolo - epsilon` | `TRIP_LOW` |
| `sample < lo - epsilon` | `WARN_LOW` |
| `sample > hihi + epsilon` | `TRIP_HIGH` |
| `sample > hi + epsilon` | `WARN_HIGH` |
| otherwise | `OK` |

The first matching row wins, so a trip outranks the warning on the same side. Every comparison is strict: a sample exactly at a limit, or within `epsilon` of it, is on the safe side.

<!-- snippet: skip -->
```c
typedef enum {
        THRESHOLD_SEV_OK = 0,
        THRESHOLD_SEV_WARN_LOW = 1,
        THRESHOLD_SEV_WARN_HIGH = 2,
        THRESHOLD_SEV_TRIP_LOW = 3,
        THRESHOLD_SEV_TRIP_HIGH = 4,
        THRESHOLD_SEV_INVALID = 5,
} threshold_severity_t;
```

The values are stable, so a severity can be stored or sent as an integer.

## Hysteresis

<!-- snippet: body -->
```c
static threshold_severity_t motor_temp_sev = THRESHOLD_SEV_OK;

motor_temp_sev = threshold_plan_eval_hys(&motor_temp_plan, 84.0f, 2.0f,
                                         motor_temp_sev);
```

```c
threshold_severity_t threshold_plan_eval_hys(const threshold_plan_t *plan,
                                             float sample, float hysteresis,
                                             threshold_severity_t prev);
```

`threshold_plan_eval_hys` evaluates like `threshold_plan_eval`, except that the limit `prev` crossed moves inwards by `hysteresis`, so a level clears only once the sample has come back past the limit by that margin:

| `prev` | Adjusted limit |
| --- | --- |
| `TRIP_HIGH` | `hihi - hysteresis` |
| `WARN_HIGH` | `hi - hysteresis` |
| `TRIP_LOW` | `lolo + hysteresis` |
| `WARN_LOW` | `lo + hysteresis` |
| `OK`, `INVALID` | none |

With the motor plan above, a hysteresis of 2 and `prev` of `WARN_HIGH`, a sample of 84 still returns `WARN_HIGH`, and 82 returns `OK`. Only the limit for `prev` moves. After a trip, the result drops to a warning once the sample is below `hihi - hysteresis`; the next call, with `prev` of `WARN_HIGH`, then applies the margin to `hi`.

The function keeps no state. The caller stores the result and passes it back as `prev` on the next call, one stored severity per monitored signal. It returns `INVALID` for a NULL or invalid plan and for a `hysteresis` that is NaN, infinite or negative, then handles `NONE` plans and non-finite samples as `threshold_plan_eval` does. A hysteresis of zero gives the same result as `threshold_plan_eval`.

## Severity helpers

```c
static inline bool threshold_severity_is_warn(threshold_severity_t sev);
static inline bool threshold_severity_is_trip(threshold_severity_t sev);
static inline bool threshold_severity_is_low(threshold_severity_t sev);
static inline bool threshold_severity_is_high(threshold_severity_t sev);
```

Each returns `true` for the two severities its name describes: `is_warn` for `WARN_LOW` and `WARN_HIGH`, `is_trip` for `TRIP_LOW` and `TRIP_HIGH`, `is_low` for the two low severities and `is_high` for the two high ones. All four return `false` for `OK` and `INVALID`, so `INVALID` needs its own check wherever a policy can produce it.

## Diagnostics

<!-- snippet: body -->
```c
threshold_status_t status = threshold_plan_build(&motor_temp_plan, &motor_temp_cfg);
const char *why = threshold_status_str(status); /* "OK" */
const char *kind = threshold_type_name(motor_temp_cfg.type); /* "UPPER" */
(void)why;
(void)kind;
```

```c
const char *threshold_severity_name(threshold_severity_t sev);
const char *threshold_type_name(threshold_type_t type);
const char *threshold_status_str(threshold_status_t status);
```

`threshold_severity_name` and `threshold_type_name` return the enumerator name without its prefix, such as `"WARN_HIGH"` or `"DISCRETE_TRIP"`. `threshold_status_str` returns a sentence, such as `"Required threshold is missing"`. Each returns a string literal and never NULL; a value outside the enumeration gives `"UNKNOWN"`, or `"Unknown status"` for a status.

| Status | Value | Meaning |
| --- | --- | --- |
| `THRESHOLD_STATUS_OK` | 0 | Success. |
| `THRESHOLD_STATUS_INVALID_ARG` | -1 | NULL pointer, bad epsilon, or unknown plan type. |
| `THRESHOLD_STATUS_OUT_OF_RANGE` | -3 | A limit's magnitude exceeds `THRESHOLD_EVAL_MAX`. |
| `THRESHOLD_STATUS_MISSING` | -4 | A limit the type needs is not finite. |
| `THRESHOLD_STATUS_ORDER` | -5 | Limits out of order under `STRICT_CONFIG`. |
| `THRESHOLD_STATUS_EXTRA` | -6 | A limit the type does not use is set under `STRICT_CONFIG`. |

## Configuration macros

| Macro | Default | Meaning |
| --- | --- | --- |
| `THRESHOLD_EVAL_MAX` | `4000000.0f` | Largest accepted limit magnitude. |
| `THRESHOLD_EVAL_EPSILON` | `1.0e-3f` | The `epsilon` that `threshold_config_init` sets. |

Both are read only when the library is compiled, so define them with `-D` for the library build; [integration.md](integration.md#configuration) explains why a definition before the include has no effect. The per-plan `epsilon` field overrides the default for one plan. [Floating-point limits](design.md#floating-point-limits) explains when a small epsilon has no effect.

## Recipes

### Supply voltage band

```c
static threshold_plan_t supply_plan;

bool
supply_configure(void)
{
        threshold_config_t cfg;

        (void)threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.5f;
        cfg.lo = 11.0f;
        cfg.hi = 14.5f;
        cfg.hihi = 15.0f;
        cfg.epsilon = 0.05f; /* the measurement noise floor */
        return threshold_plan_build(&supply_plan, &cfg) == THRESHOLD_STATUS_OK;
}
```

### Informational input

A coolant flow reading that should never stop the machine: a lost sensor reports a warning, and limits entered in any order are sorted.

```c
static threshold_plan_t flow_plan;

bool
flow_configure(float low_trip, float low_warn)
{
        threshold_config_t cfg;

        (void)threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = low_trip;
        cfg.lo = low_warn;
        cfg.policy = THRESHOLD_POLICY_OPERATIONAL;
        return threshold_plan_build(&flow_plan, &cfg) == THRESHOLD_STATUS_OK;
}
```

### Single limit on a derived value

```c
static threshold_plan_t imbalance_plan;

bool
imbalance_configure(void)
{
        threshold_config_t cfg;

        (void)threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 80.0f; /* warn above 80 percent imbalance */
        return threshold_plan_build(&imbalance_plan, &cfg) == THRESHOLD_STATUS_OK;
}
```

### Several signals, one table

```c
struct monitor {
        threshold_plan_t plan;
        threshold_severity_t sev;
        float hysteresis;
};

/* Returns true if any monitor is tripped. */
bool
monitors_poll(struct monitor *mon, const float *samples, unsigned count)
{
        bool tripped = false;

        for (unsigned i = 0u; i < count; ++i) {
                mon[i].sev = threshold_plan_eval_hys(&mon[i].plan, samples[i],
                                                     mon[i].hysteresis,
                                                     mon[i].sev);
                tripped = tripped || threshold_severity_is_trip(mon[i].sev);
        }
        return tripped;
}
```

## Version

`threshold_eval.h` defines the release it belongs to, so a copied-in header still identifies itself:

<!-- snippet: skip -->
```c
#define THRESHOLD_EVAL_VERSION_MAJOR 1
#define THRESHOLD_EVAL_VERSION_MINOR 3
#define THRESHOLD_EVAL_VERSION_PATCH 0
#define THRESHOLD_EVAL_VERSION "1.3.0"
```

`THRESHOLD_EVAL_VERSION_NUMBER` is `MAJOR * 10000 + MINOR * 100 + PATCH` as a `long`, so it holds any version where `int` is 16 bits. Compare it in the preprocessor to require a minimum release:

```c
#if THRESHOLD_EVAL_VERSION_NUMBER < 10300L
#error "threshold_eval 1.3.0 or later is required"
#endif
```

`THRESHOLD_EVAL_VERSION_STRING` is the same string as `THRESHOLD_EVAL_VERSION`.
