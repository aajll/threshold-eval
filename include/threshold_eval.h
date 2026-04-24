/**
 * @file threshold_eval.h
 * @brief Public API for threshold-eval.
 *
 * @details
 *   This library provides a configurable, deterministic mechanism for
 *   evaluating measured values against threshold limits (LOLO, LO, HI, HIHI).
 *   It is designed for use in safety-critical embedded systems targeting
 *   IEC 61508 SIL 2 and MISRA C:2012 compliance.
 *
 *   Key design principles:
 *   - No dynamic memory allocation
 *   - No hidden global/static state
 *   - Explicit configuration validation separate from runtime evaluation
 *   - Deterministic, bounded execution
 *   - Configurable failsafe policies for sensor failures and invalid data
 *   - Support for hysteresis to prevent threshold chatter
 *
 * @note
 *   This library uses single-precision floating-point for thresholds and
 *   samples. All floating-point inputs are validated for finiteness per
 *   MISRA C:2012 Dir 4.15 (no reliance on NaN/Inf behavior).
 *
 *   Configuration macros (THRESHOLD_EVAL_MAX, THRESHOLD_EVAL_EPSILON) are
 *   defined in threshold_eval_conf.h, which is included automatically.
 *   Override them before including this header or threshold_eval_conf.h.
 */

#ifndef THRESHOLD_EVAL_H_
#define THRESHOLD_EVAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ================ INCLUDES ================================================ */

#include <stdbool.h>
#include <stdint.h>

#include "threshold_eval_conf.h"

/* ================ TYPEDEFS ================================================ */

/**
 * @enum threshold_type_t
 * @brief Defines which thresholds are active for an evaluation plan.
 *
 * @details
 *   The type determines which of the four threshold fields (lolo, lo, hi,
 *   hihi) are required in the configuration and used during evaluation.
 */
typedef enum {
        /** No threshold checking; all samples return THRESHOLD_SEV_OK. */
        THRESHOLD_TYPE_NONE = 0,

        /** Full range: LOLO < LO < HI < HIHI (e.g., voltage, frequency). */
        THRESHOLD_TYPE_RANGE = 1,

        /** Upper only: HI, HIHI (e.g., temperature, imbalance percentage). */
        THRESHOLD_TYPE_UPPER = 2,

        /** Lower only: LOLO, LO (e.g., minimum pressure, minimum speed). */
        THRESHOLD_TYPE_LOWER = 3,

        /** Discrete warning: single HI threshold triggers warning. */
        THRESHOLD_TYPE_DISCRETE_WARN = 4,

        /** Discrete trip: single HIHI threshold triggers trip. */
        THRESHOLD_TYPE_DISCRETE_TRIP = 5,
} threshold_type_t;

/**
 * @enum threshold_policy_t
 * @brief Policy flags controlling threshold evaluation behavior.
 *
 * @details
 *   Multiple flags can be combined with bitwise OR. Conflicting invalid-sample
 *   policies (FAILSAFE_TRIP, DEESCALATE_WARN, IGNORE_INVALID) are resolved in
 *   priority order: FAILSAFE_TRIP > DEESCALATE_WARN > IGNORE_INVALID.
 */
typedef enum {
        /** No special policy; invalid samples return THRESHOLD_SEV_INVALID. */
        THRESHOLD_POLICY_NONE = 0,

        /**
         * Failsafe: treat invalid samples (NaN, Inf) as a trip condition.
         * Use this for safety-critical parameters where sensor failure
         * must trigger protective action.
         */
        THRESHOLD_POLICY_FAILSAFE_TRIP = (1u << 0),

        /**
         * De-escalate: treat invalid samples as a warning instead of trip.
         * Use this when operation must continue despite sensor issues,
         * with operator notification.
         */
        THRESHOLD_POLICY_DEESCALATE_WARN = (1u << 1),

        /**
         * Ignore invalid: treat invalid samples as OK.
         * Use this only for non-critical informational parameters.
         * @warning This policy should be used with extreme caution in
         *          safety-critical applications.
         */
        THRESHOLD_POLICY_IGNORE_INVALID = (1u << 2),

        /**
         * Strict configuration: reject plans with any configuration issues
         * (missing thresholds, bad ordering, out-of-range values).
         * Recommended for production systems.
         */
        THRESHOLD_POLICY_STRICT_CONFIG = (1u << 3),

        /**
         * Allow threshold reordering: during plan build, automatically
         * sort thresholds into monotonic order (LOLO <= LO <= HI <= HIHI).
         * Only applied if STRICT_CONFIG is NOT set.
         */
        THRESHOLD_POLICY_ALLOW_REORDER = (1u << 4),
} threshold_policy_t;

/**
 * @def THRESHOLD_POLICY_FAILSAFE
 * @brief Recommended policy for safety-critical parameters.
 *
 * @details
 *   Combines FAILSAFE_TRIP (sensor failures trip the parameter) with
 *   STRICT_CONFIG (configuration errors are rejected at plan build time).
 *   Use this as the default for any parameter that drives protective action.
 */
#define THRESHOLD_POLICY_FAILSAFE                                              \
        ((uint32_t)(THRESHOLD_POLICY_FAILSAFE_TRIP                             \
                    | THRESHOLD_POLICY_STRICT_CONFIG))

/**
 * @def THRESHOLD_POLICY_OPERATIONAL
 * @brief Policy for parameters requiring continued operation with degraded
 *        monitoring.
 *
 * @details
 *   Combines DEESCALATE_WARN (sensor failures produce warnings, not trips)
 *   with ALLOW_REORDER (threshold ordering is corrected automatically).
 *   Use this for informational parameters where operation must continue.
 */
#define THRESHOLD_POLICY_OPERATIONAL                                           \
        ((uint32_t)(THRESHOLD_POLICY_DEESCALATE_WARN                           \
                    | THRESHOLD_POLICY_ALLOW_REORDER))

/**
 * @enum threshold_severity_t
 * @brief Result of evaluating a sample against configured thresholds.
 *
 * @details
 *   Severity levels are ordered: OK < WARN_LOW/HIGH < TRIP_LOW/HIGH <
 *   INVALID. The monitor layer uses these to drive debounce, latching, and
 *   protective actions.
 */
typedef enum {
        /** Sample is within normal operating range. */
        THRESHOLD_SEV_OK = 0,

        /** Sample is below LO threshold (warning). */
        THRESHOLD_SEV_WARN_LOW = 1,

        /** Sample is above HI threshold (warning). */
        THRESHOLD_SEV_WARN_HIGH = 2,

        /** Sample is below LOLO threshold (trip). */
        THRESHOLD_SEV_TRIP_LOW = 3,

        /** Sample is above HIHI threshold (trip). */
        THRESHOLD_SEV_TRIP_HIGH = 4,

        /**
         * Sample is invalid (non-finite: NaN or Inf).
         * The configured policy determines how this is handled by the
         * evaluation functions.
         */
        THRESHOLD_SEV_INVALID = 5,
} threshold_severity_t;

/**
 * @enum threshold_status_t
 * @brief Status codes returned by library functions.
 */
typedef enum {
        /** Operation succeeded. */
        THRESHOLD_STATUS_OK = 0,

        /** Invalid argument (NULL pointer, invalid enum value). */
        THRESHOLD_STATUS_INVALID_ARG = -1,

        /** Threshold value exceeds THRESHOLD_EVAL_MAX. */
        THRESHOLD_STATUS_OUT_OF_RANGE = -3,

        /** Required threshold is missing for the specified type. */
        THRESHOLD_STATUS_MISSING = -4,

        /**
         * Thresholds are not in monotonic order (LOLO <= LO <= HI <= HIHI).
         * Only returned when THRESHOLD_POLICY_STRICT_CONFIG is set and
         * THRESHOLD_POLICY_ALLOW_REORDER is not set.
         */
        THRESHOLD_STATUS_ORDER = -5,

        /**
         * Extra thresholds provided that don't apply to the plan type.
         * Only returned when THRESHOLD_POLICY_STRICT_CONFIG is set.
         */
        THRESHOLD_STATUS_EXTRA = -6,
} threshold_status_t;

/**
 * @struct threshold_config_t
 * @brief User-provided threshold configuration.
 *
 * @details
 *   Populate this structure with the desired thresholds, plan type, and policy,
 *   then pass to threshold_plan_build() for validation.
 *
 *   The four threshold fields (lolo, lo, hi, hihi) map to the standard
 *   process industry convention:
 *   @code
 *   TRIP_LOW   WARN_LOW       OK         WARN_HIGH  TRIP_HIGH
 *   |<-------->|<------------>|<--------->|<-------->|
 *   lolo       lo             ...         hi         hihi
 *   @endcode
 *
 * @note
 *   For types that don't use all four thresholds, unused fields should be
 *   set to NAN (the default after threshold_config_init()). If
 *   THRESHOLD_POLICY_STRICT_CONFIG is set, extra finite thresholds for a
 *   type that doesn't use them will cause threshold_plan_build() to fail
 *   with THRESHOLD_STATUS_EXTRA.
 */
typedef struct {
        /** Low-low trip threshold. Required for RANGE and LOWER types. */
        float lolo;

        /** Low warning threshold. Required for RANGE and LOWER types. */
        float lo;

        /**
         * High warning threshold.
         * Required for RANGE, UPPER, and DISCRETE_WARN types.
         */
        float hi;

        /**
         * High-high trip threshold.
         * Required for RANGE, UPPER, and DISCRETE_TRIP types.
         */
        float hihi;

        /**
         * Comparison epsilon for threshold boundary checks (>= 0).
         * Defaults to THRESHOLD_EVAL_EPSILON after threshold_config_init().
         */
        float epsilon;

        /** Plan type defining which thresholds are active. */
        threshold_type_t type;

        /** Policy flags controlling evaluation behavior. */
        uint32_t policy;
} threshold_config_t;

/**
 * @struct threshold_plan_t
 * @brief Validated, normalized threshold evaluation plan.
 *
 * @details
 *   This structure is populated by threshold_plan_build() and consumed by
 *   threshold_plan_eval() and threshold_plan_eval_hys(). It holds a
 *   validated, optionally reordered copy of the configuration thresholds.
 *
 * @note
 *   Do not modify this structure directly after it has been built.
 *   Always use threshold_plan_build() to create or update a plan.
 *   The plan is only safe for evaluation when plan->valid is true.
 */
typedef struct {
        /** Validated low-low trip threshold. */
        float lolo;

        /** Validated low warning threshold. */
        float lo;

        /** Validated high warning threshold. */
        float hi;

        /** Validated high-high trip threshold. */
        float hihi;

        /** Comparison epsilon (validated, >= 0). */
        float epsilon;

        /** Plan type (validated). */
        threshold_type_t type;

        /** Policy flags (validated). */
        uint32_t policy;

        /** True if plan was successfully built and is valid for evaluation. */
        bool valid;
} threshold_plan_t;

/* ================ MACROS ================================================== */

/* ================ GLOBAL VARIABLES ======================================== */

/* ================ GLOBAL PROTOTYPES ======================================= */

/**
 * @brief Initialise a threshold configuration with safe defaults.
 *
 * @details
 *   Sets all thresholds to NAN (unused), type to THRESHOLD_TYPE_NONE,
 *   policy to THRESHOLD_POLICY_FAILSAFE, and epsilon to
 *   THRESHOLD_EVAL_EPSILON. After initialisation the config is ready for
 *   field-by-field population before being passed to threshold_plan_build().
 *
 * @param[out] config  Configuration to initialise. Must not be NULL.
 *
 * @return THRESHOLD_STATUS_OK on success.
 * @return THRESHOLD_STATUS_INVALID_ARG if config is NULL.
 *
 * @pre  config points to valid, writable memory.
 * @post config is in a known initial state suitable for modification.
 * @post All threshold fields are NAN; type is NONE; policy is FAILSAFE.
 */
threshold_status_t threshold_config_init(threshold_config_t *config);

/**
 * @brief Build and validate a threshold evaluation plan from configuration.
 *
 * @details
 *   Validates all configuration parameters and, if valid, populates the plan
 *   structure for use in evaluation. The validation checks:
 *   - All required thresholds for the plan type are present and finite
 *   - Thresholds are within [-THRESHOLD_EVAL_MAX, +THRESHOLD_EVAL_MAX]
 *   - Thresholds are in monotonic order (or reorders if policy allows)
 *   - No extra thresholds if THRESHOLD_POLICY_STRICT_CONFIG is set
 *   - Epsilon is non-negative and finite
 *
 * @param[out] plan    Plan to populate. Must not be NULL.
 * @param[in]  config  Configuration to validate. Must not be NULL.
 *
 * @return THRESHOLD_STATUS_OK on success.
 * @return Negative error code on failure (see threshold_status_t).
 *
 * @pre  plan and config point to valid memory.
 * @pre  config was populated (manually or via threshold_config_init()).
 * @post On success, plan->valid is true and plan is ready for evaluation.
 * @post On failure, plan->valid is false; plan must not be used for eval.
 *
 * @note
 *   If THRESHOLD_POLICY_ALLOW_REORDER is set and thresholds are out of
 *   order, they are sorted into monotonic order in the plan. The original
 *   config is never modified.
 * @note
 *   THRESHOLD_POLICY_STRICT_CONFIG and THRESHOLD_POLICY_ALLOW_REORDER are
 *   mutually exclusive for ordering checks; ALLOW_REORDER takes precedence.
 */
threshold_status_t threshold_plan_build(threshold_plan_t *plan,
                                        const threshold_config_t *config);

/**
 * @brief Evaluate a sample value against a validated threshold plan.
 *
 * @details
 *   Performs instantaneous threshold evaluation without hysteresis.
 *   The evaluation is stateless: no side effects, no persistent state.
 *
 *   Comparison boundaries are adjusted by epsilon:
 *   - Trip low:  sample < lolo - epsilon
 *   - Warn low:  sample < lo   - epsilon
 *   - Warn high: sample > hi   + epsilon
 *   - Trip high: sample > hihi + epsilon
 *
 * @param[in] plan   Validated threshold plan. Must not be NULL and must be
 *                   valid (plan->valid == true).
 * @param[in] sample The measured value to evaluate.
 *
 * @return Severity level (THRESHOLD_SEV_OK through THRESHOLD_SEV_INVALID).
 * @return THRESHOLD_SEV_INVALID if plan is NULL or plan->valid is false.
 *
 * @pre  plan was successfully built via threshold_plan_build().
 * @post No state is modified; this function is pure/stateless.
 *
 * @note
 *   For THRESHOLD_TYPE_NONE, always returns THRESHOLD_SEV_OK regardless of
 *   the sample value.
 * @note
 *   For non-finite samples (NaN, Inf), the result is determined by the
 *   configured policy flags (FAILSAFE_TRIP, DEESCALATE_WARN, IGNORE_INVALID).
 *   With no policy flags, THRESHOLD_SEV_INVALID is returned.
 */
threshold_severity_t threshold_plan_eval(const threshold_plan_t *plan,
                                         float sample);

/**
 * @brief Evaluate a sample with hysteresis based on previous severity.
 *
 * @details
 *   Adjusts active thresholds based on the previous evaluation severity to
 *   prevent chatter when a value oscillates near a threshold boundary.
 *
 *   Hysteresis is applied by moving the relevant threshold inward:
 *   - If prev == THRESHOLD_SEV_TRIP_HIGH: HIHI is lowered by hysteresis
 *   - If prev == THRESHOLD_SEV_WARN_HIGH: HI   is lowered by hysteresis
 *   - If prev == THRESHOLD_SEV_TRIP_LOW:  LOLO is raised  by hysteresis
 *   - If prev == THRESHOLD_SEV_WARN_LOW:  LO   is raised  by hysteresis
 *   - Otherwise: thresholds are unadjusted (same as threshold_plan_eval)
 *
 *   The caller is responsible for tracking the previous severity between
 *   calls. This function is stateless; it does not store prev internally.
 *
 * @param[in] plan       Validated threshold plan. Must not be NULL and valid.
 * @param[in] sample     The measured value to evaluate.
 * @param[in] hysteresis Hysteresis band width (must be >= 0 and finite).
 * @param[in] prev       Severity result from the previous evaluation call.
 *
 * @return Severity level after hysteresis-adjusted evaluation.
 * @return THRESHOLD_SEV_INVALID if plan is NULL/invalid or hysteresis < 0
 *         or hysteresis is non-finite.
 *
 * @pre  plan was successfully built via threshold_plan_build().
 * @pre  prev is the result of the most recent threshold_plan_eval() or
 *       threshold_plan_eval_hys() call for this signal.
 * @post No state is modified; caller must track prev between calls.
 */
threshold_severity_t threshold_plan_eval_hys(const threshold_plan_t *plan,
                                             float sample, float hysteresis,
                                             threshold_severity_t prev);

/**
 * @brief Check if a severity indicates a warning condition.
 *
 * @param[in] sev  Severity to check.
 *
 * @return true if sev is THRESHOLD_SEV_WARN_LOW or THRESHOLD_SEV_WARN_HIGH.
 */
static inline bool
threshold_severity_is_warn(threshold_severity_t sev)
{
        return (sev == THRESHOLD_SEV_WARN_LOW)
               || (sev == THRESHOLD_SEV_WARN_HIGH);
}

/**
 * @brief Check if a severity indicates a trip condition.
 *
 * @param[in] sev  Severity to check.
 *
 * @return true if sev is THRESHOLD_SEV_TRIP_LOW or THRESHOLD_SEV_TRIP_HIGH.
 */
static inline bool
threshold_severity_is_trip(threshold_severity_t sev)
{
        return (sev == THRESHOLD_SEV_TRIP_LOW)
               || (sev == THRESHOLD_SEV_TRIP_HIGH);
}

/**
 * @brief Check if a severity indicates a low (below threshold) condition.
 *
 * @param[in] sev  Severity to check.
 *
 * @return true if sev is THRESHOLD_SEV_WARN_LOW or THRESHOLD_SEV_TRIP_LOW.
 */
static inline bool
threshold_severity_is_low(threshold_severity_t sev)
{
        return (sev == THRESHOLD_SEV_WARN_LOW)
               || (sev == THRESHOLD_SEV_TRIP_LOW);
}

/**
 * @brief Check if a severity indicates a high (above threshold) condition.
 *
 * @param[in] sev  Severity to check.
 *
 * @return true if sev is THRESHOLD_SEV_WARN_HIGH or THRESHOLD_SEV_TRIP_HIGH.
 */
static inline bool
threshold_severity_is_high(threshold_severity_t sev)
{
        return (sev == THRESHOLD_SEV_WARN_HIGH)
               || (sev == THRESHOLD_SEV_TRIP_HIGH);
}

/**
 * @brief Get a human-readable name for a severity level.
 *
 * @param[in] sev  Severity level.
 *
 * @return Pointer to a static string (e.g., "OK", "WARN_LOW", "TRIP_HIGH").
 *         Returns "UNKNOWN" for unrecognized values. Never returns NULL.
 */
const char *threshold_severity_name(threshold_severity_t sev);

/**
 * @brief Get a human-readable name for a plan type.
 *
 * @param[in] type  Plan type.
 *
 * @return Pointer to a static string (e.g., "RANGE", "UPPER", "LOWER").
 *         Returns "UNKNOWN" for unrecognized values. Never returns NULL.
 */
const char *threshold_type_name(threshold_type_t type);

/**
 * @brief Get a human-readable description of a status code.
 *
 * @param[in] status  Status code.
 *
 * @return Pointer to a static description string. Never returns NULL.
 */
const char *threshold_status_str(threshold_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* THRESHOLD_EVAL_H_ */
