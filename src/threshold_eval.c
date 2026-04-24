/**
 * @file threshold_eval.c
 * @brief Implementation of threshold-eval.
 */

/* ================ INCLUDES ================================================ */

#include <math.h>
#include <stddef.h>

#include "threshold_eval.h"

/* ================ DEFINES ================================================= */

/* ================ STRUCTURES ============================================== */

/* ================ TYPEDEFS ================================================ */

/* ================ STATIC PROTOTYPES ======================================= */

/* ================ STATIC VARIABLES ======================================== */

/* ================ MACROS ================================================== */

/* ================ STATIC FUNCTIONS ======================================== */

/* ---------------- Helpers ------------------------------------------------- */

/**
 * @brief Check if a float value is finite (not NaN or Inf).
 *
 * Wrapper for standard isfinite() to ensure consistent behavior across
 * compilers and avoid reliance on non-standard extensions.
 */
static inline bool
is_finite(float val)
{
        return isfinite(val) != 0;
}

/**
 * @brief Check if a threshold value is within the configured valid range.
 *
 * A threshold is valid if it is finite and its absolute value does not
 * exceed THRESHOLD_EVAL_MAX.
 *
 * @param[in] val  The threshold value to check.
 *
 * @return true if val is finite and within [-THRESHOLD_EVAL_MAX,
 *         +THRESHOLD_EVAL_MAX].
 */
static inline bool
threshold_in_range(float val)
{
        if (!is_finite(val)) {
                return false;
        }
        const float abs_val = (val < 0.0f) ? -val : val;
        return abs_val <= THRESHOLD_EVAL_MAX;
}

/**
 * @brief Swap two float values in place.
 *
 * @param[in,out] a  First value.
 * @param[in,out] b  Second value.
 */
static inline void
swap_floats(float *a, float *b)
{
        const float tmp = *a;
        *a = *b;
        *b = tmp;
}

/**
 * @brief Sort an array of floats in ascending order using insertion sort.
 *
 * @details
 *   Insertion sort is used because it is optimal for small arrays (n <= 4),
 *   has no recursive calls, and is simple to verify for correctness.
 *
 * @param[in,out] arr  Array of floats to sort in place. May be NULL.
 * @param[in]     n    Number of elements. No-op if n < 2.
 */
static void
sort_floats_ascending(float *arr, size_t n)
{
        if ((arr == NULL) || (n < 2u)) {
                return;
        }

        for (size_t i = 1u; i < n; i++) {
                size_t j = i;
                while ((j > 0u) && (arr[j - 1u] > arr[j])) {
                        swap_floats(&arr[j - 1u], &arr[j]);
                        j--;
                }
        }
}

/**
 * @brief Check if two floats satisfy monotonic ordering (a <= b).
 *
 * @param[in] a  Lower value.
 * @param[in] b  Upper value.
 *
 * @return true if a <= b.
 */
static inline bool
is_ordered(float a, float b)
{
        return a <= b;
}

/**
 * @brief Resolve the severity for a non-finite sample based on policy flags.
 *
 * @details
 *   Priority order when multiple flags are set:
 *   FAILSAFE_TRIP > DEESCALATE_WARN > IGNORE_INVALID.
 *   If no relevant flag is set, returns THRESHOLD_SEV_INVALID.
 *
 * @param[in] policy  Policy bitmask from the evaluation plan.
 *
 * @return Resolved severity for the invalid sample.
 */
static threshold_severity_t
apply_invalid_policy(uint32_t policy)
{
        if ((policy & THRESHOLD_POLICY_FAILSAFE_TRIP) != 0u) {
                return THRESHOLD_SEV_TRIP_HIGH;
        }
        if ((policy & THRESHOLD_POLICY_DEESCALATE_WARN) != 0u) {
                return THRESHOLD_SEV_WARN_HIGH;
        }
        if ((policy & THRESHOLD_POLICY_IGNORE_INVALID) != 0u) {
                return THRESHOLD_SEV_OK;
        }
        return THRESHOLD_SEV_INVALID;
}

/* ---------------- Validation ---------------------------------------------- */

/**
 * @brief Validate thresholds for THRESHOLD_TYPE_RANGE.
 *
 * Requires all four thresholds (lolo, lo, hi, hihi) to be finite and within
 * range. If strict is true, also checks monotonic order.
 *
 * @param[in] cfg     Configuration to validate.
 * @param[in] strict  If true, reject out-of-order thresholds.
 *
 * @return THRESHOLD_STATUS_OK on success, negative status on failure.
 */
static threshold_status_t
validate_range(const threshold_config_t *cfg, bool strict)
{
        if (!is_finite(cfg->lolo) || !is_finite(cfg->lo) || !is_finite(cfg->hi)
            || !is_finite(cfg->hihi)) {
                return THRESHOLD_STATUS_MISSING;
        }

        if (!threshold_in_range(cfg->lolo) || !threshold_in_range(cfg->lo)
            || !threshold_in_range(cfg->hi) || !threshold_in_range(cfg->hihi)) {
                return THRESHOLD_STATUS_OUT_OF_RANGE;
        }

        if (strict) {
                if (!is_ordered(cfg->lolo, cfg->lo)
                    || !is_ordered(cfg->lo, cfg->hi)
                    || !is_ordered(cfg->hi, cfg->hihi)) {
                        return THRESHOLD_STATUS_ORDER;
                }
        }

        return THRESHOLD_STATUS_OK;
}

/**
 * @brief Validate thresholds for THRESHOLD_TYPE_UPPER.
 *
 * Requires hi and hihi. If strict, rejects extra lower thresholds and
 * verifies hi <= hihi.
 *
 * @param[in] cfg     Configuration to validate.
 * @param[in] strict  If true, apply extra-threshold and ordering checks.
 *
 * @return THRESHOLD_STATUS_OK on success, negative status on failure.
 */
static threshold_status_t
validate_upper(const threshold_config_t *cfg, bool strict)
{
        if (!is_finite(cfg->hi) || !is_finite(cfg->hihi)) {
                return THRESHOLD_STATUS_MISSING;
        }

        if (!threshold_in_range(cfg->hi) || !threshold_in_range(cfg->hihi)) {
                return THRESHOLD_STATUS_OUT_OF_RANGE;
        }

        if (strict) {
                if (is_finite(cfg->lolo) || is_finite(cfg->lo)) {
                        return THRESHOLD_STATUS_EXTRA;
                }
        }

        if (strict) {
                if (!is_ordered(cfg->hi, cfg->hihi)) {
                        return THRESHOLD_STATUS_ORDER;
                }
        }

        return THRESHOLD_STATUS_OK;
}

/**
 * @brief Validate thresholds for THRESHOLD_TYPE_LOWER.
 *
 * Requires lolo and lo. If strict, rejects extra upper thresholds and
 * verifies lolo <= lo.
 *
 * @param[in] cfg     Configuration to validate.
 * @param[in] strict  If true, apply extra-threshold and ordering checks.
 *
 * @return THRESHOLD_STATUS_OK on success, negative status on failure.
 */
static threshold_status_t
validate_lower(const threshold_config_t *cfg, bool strict)
{
        if (!is_finite(cfg->lolo) || !is_finite(cfg->lo)) {
                return THRESHOLD_STATUS_MISSING;
        }

        if (!threshold_in_range(cfg->lolo) || !threshold_in_range(cfg->lo)) {
                return THRESHOLD_STATUS_OUT_OF_RANGE;
        }

        if (strict) {
                if (is_finite(cfg->hi) || is_finite(cfg->hihi)) {
                        return THRESHOLD_STATUS_EXTRA;
                }
        }

        if (strict) {
                if (!is_ordered(cfg->lolo, cfg->lo)) {
                        return THRESHOLD_STATUS_ORDER;
                }
        }

        return THRESHOLD_STATUS_OK;
}

/**
 * @brief Validate thresholds for THRESHOLD_TYPE_DISCRETE_WARN.
 *
 * Requires only hi. If strict, rejects all other threshold fields.
 *
 * @param[in] cfg     Configuration to validate.
 * @param[in] strict  If true, reject extra threshold fields.
 *
 * @return THRESHOLD_STATUS_OK on success, negative status on failure.
 */
static threshold_status_t
validate_discrete_warn(const threshold_config_t *cfg, bool strict)
{
        if (!is_finite(cfg->hi)) {
                return THRESHOLD_STATUS_MISSING;
        }

        if (!threshold_in_range(cfg->hi)) {
                return THRESHOLD_STATUS_OUT_OF_RANGE;
        }

        if (strict) {
                if (is_finite(cfg->lolo) || is_finite(cfg->lo)
                    || is_finite(cfg->hihi)) {
                        return THRESHOLD_STATUS_EXTRA;
                }
        }

        return THRESHOLD_STATUS_OK;
}

/**
 * @brief Validate thresholds for THRESHOLD_TYPE_DISCRETE_TRIP.
 *
 * Requires only hihi. If strict, rejects all other threshold fields.
 *
 * @param[in] cfg     Configuration to validate.
 * @param[in] strict  If true, reject extra threshold fields.
 *
 * @return THRESHOLD_STATUS_OK on success, negative status on failure.
 */
static threshold_status_t
validate_discrete_trip(const threshold_config_t *cfg, bool strict)
{
        if (!is_finite(cfg->hihi)) {
                return THRESHOLD_STATUS_MISSING;
        }

        if (!threshold_in_range(cfg->hihi)) {
                return THRESHOLD_STATUS_OUT_OF_RANGE;
        }

        if (strict) {
                if (is_finite(cfg->lolo) || is_finite(cfg->lo)
                    || is_finite(cfg->hi)) {
                        return THRESHOLD_STATUS_EXTRA;
                }
        }

        return THRESHOLD_STATUS_OK;
}

/* ---------------- Evaluation ---------------------------------------------- */

/**
 * @brief Evaluate THRESHOLD_TYPE_RANGE against all four thresholds.
 *
 * Checks lolo, lo, hi, hihi with epsilon adjustment. TRIP_LOW takes
 * priority over WARN_LOW; TRIP_HIGH takes priority over WARN_HIGH.
 *
 * @param[in] plan    Validated plan with RANGE type.
 * @param[in] sample  Finite sample value to evaluate.
 *
 * @return Resolved severity.
 */
static threshold_severity_t
eval_range(const threshold_plan_t *plan, float sample)
{
        const float eps = plan->epsilon;

        if (sample < plan->lolo - eps) {
                return THRESHOLD_SEV_TRIP_LOW;
        }
        if (sample < plan->lo - eps) {
                return THRESHOLD_SEV_WARN_LOW;
        }
        if (sample > plan->hihi + eps) {
                return THRESHOLD_SEV_TRIP_HIGH;
        }
        if (sample > plan->hi + eps) {
                return THRESHOLD_SEV_WARN_HIGH;
        }
        return THRESHOLD_SEV_OK;
}

/**
 * @brief Evaluate THRESHOLD_TYPE_UPPER against hi and hihi.
 *
 * @param[in] plan    Validated plan with UPPER type.
 * @param[in] sample  Finite sample value to evaluate.
 *
 * @return Resolved severity.
 */
static threshold_severity_t
eval_upper(const threshold_plan_t *plan, float sample)
{
        const float eps = plan->epsilon;

        if (sample > plan->hihi + eps) {
                return THRESHOLD_SEV_TRIP_HIGH;
        }
        if (sample > plan->hi + eps) {
                return THRESHOLD_SEV_WARN_HIGH;
        }
        return THRESHOLD_SEV_OK;
}

/**
 * @brief Evaluate THRESHOLD_TYPE_LOWER against lolo and lo.
 *
 * @param[in] plan    Validated plan with LOWER type.
 * @param[in] sample  Finite sample value to evaluate.
 *
 * @return Resolved severity.
 */
static threshold_severity_t
eval_lower(const threshold_plan_t *plan, float sample)
{
        const float eps = plan->epsilon;

        if (sample < plan->lolo - eps) {
                return THRESHOLD_SEV_TRIP_LOW;
        }
        if (sample < plan->lo - eps) {
                return THRESHOLD_SEV_WARN_LOW;
        }
        return THRESHOLD_SEV_OK;
}

/**
 * @brief Evaluate THRESHOLD_TYPE_DISCRETE_WARN against hi only.
 *
 * @param[in] plan    Validated plan with DISCRETE_WARN type.
 * @param[in] sample  Finite sample value to evaluate.
 *
 * @return THRESHOLD_SEV_WARN_HIGH if sample > hi + epsilon, else OK.
 */
static threshold_severity_t
eval_discrete_warn(const threshold_plan_t *plan, float sample)
{
        const float eps = plan->epsilon;

        if (sample > plan->hi + eps) {
                return THRESHOLD_SEV_WARN_HIGH;
        }
        return THRESHOLD_SEV_OK;
}

/**
 * @brief Evaluate THRESHOLD_TYPE_DISCRETE_TRIP against hihi only.
 *
 * @param[in] plan    Validated plan with DISCRETE_TRIP type.
 * @param[in] sample  Finite sample value to evaluate.
 *
 * @return THRESHOLD_SEV_TRIP_HIGH if sample > hihi + epsilon, else OK.
 */
static threshold_severity_t
eval_discrete_trip(const threshold_plan_t *plan, float sample)
{
        const float eps = plan->epsilon;

        if (sample > plan->hihi + eps) {
                return THRESHOLD_SEV_TRIP_HIGH;
        }
        return THRESHOLD_SEV_OK;
}

/* ================ GLOBAL FUNCTIONS ======================================== */

threshold_status_t
threshold_config_init(threshold_config_t *config)
{
        if (config == NULL) {
                return THRESHOLD_STATUS_INVALID_ARG;
        }

        config->lolo = NAN;
        config->lo = NAN;
        config->hi = NAN;
        config->hihi = NAN;
        config->epsilon = THRESHOLD_EVAL_EPSILON;
        config->type = THRESHOLD_TYPE_NONE;
        config->policy = THRESHOLD_POLICY_FAILSAFE;

        return THRESHOLD_STATUS_OK;
}

threshold_status_t
threshold_plan_build(threshold_plan_t *plan, const threshold_config_t *config)
{
        threshold_status_t status;

        if ((plan == NULL) || (config == NULL)) {
                if (plan != NULL) {
                        plan->valid = false;
                }
                return THRESHOLD_STATUS_INVALID_ARG;
        }

        /* Mark invalid until successfully built */
        plan->valid = false;

        /* Validate plan type */
        if ((config->type < THRESHOLD_TYPE_NONE)
            || (config->type > THRESHOLD_TYPE_DISCRETE_TRIP)) {
                return THRESHOLD_STATUS_INVALID_ARG;
        }

        /* Validate epsilon */
        if (!is_finite(config->epsilon) || (config->epsilon < 0.0f)) {
                return THRESHOLD_STATUS_INVALID_ARG;
        }

        /* Copy basic fields */
        plan->type = config->type;
        plan->policy = config->policy;
        plan->epsilon = config->epsilon;

        /* THRESHOLD_TYPE_NONE requires no thresholds */
        if (config->type == THRESHOLD_TYPE_NONE) {
                plan->lolo = NAN;
                plan->lo = NAN;
                plan->hi = NAN;
                plan->hihi = NAN;
                plan->valid = true;
                return THRESHOLD_STATUS_OK;
        }

        const bool strict =
            (config->policy & THRESHOLD_POLICY_STRICT_CONFIG) != 0u;
        const bool allow_reorder =
            (config->policy & THRESHOLD_POLICY_ALLOW_REORDER) != 0u;

        /* Validate based on plan type */
        switch (config->type) {
        case THRESHOLD_TYPE_RANGE:
                status = validate_range(config, strict && !allow_reorder);
                break;
        case THRESHOLD_TYPE_UPPER:
                status = validate_upper(config, strict);
                break;
        case THRESHOLD_TYPE_LOWER:
                status = validate_lower(config, strict);
                break;
        case THRESHOLD_TYPE_DISCRETE_WARN:
                status = validate_discrete_warn(config, strict);
                break;
        case THRESHOLD_TYPE_DISCRETE_TRIP:
                status = validate_discrete_trip(config, strict);
                break;
        default: return THRESHOLD_STATUS_INVALID_ARG;
        }

        if (status != THRESHOLD_STATUS_OK) {
                return status;
        }

        /* Copy and optionally reorder thresholds into the plan */
        switch (config->type) {
        case THRESHOLD_TYPE_RANGE: {
                float thresholds[4] = {config->lolo, config->lo, config->hi,
                                       config->hihi};
                if (allow_reorder) {
                        sort_floats_ascending(thresholds, 4u);
                }
                plan->lolo = thresholds[0];
                plan->lo = thresholds[1];
                plan->hi = thresholds[2];
                plan->hihi = thresholds[3];
                break;
        }
        case THRESHOLD_TYPE_UPPER: {
                float thresholds[2] = {config->hi, config->hihi};
                if (allow_reorder && (thresholds[0] > thresholds[1])) {
                        swap_floats(&thresholds[0], &thresholds[1]);
                }
                plan->lolo = NAN;
                plan->lo = NAN;
                plan->hi = thresholds[0];
                plan->hihi = thresholds[1];
                break;
        }
        case THRESHOLD_TYPE_LOWER: {
                float thresholds[2] = {config->lolo, config->lo};
                if (allow_reorder && (thresholds[0] > thresholds[1])) {
                        swap_floats(&thresholds[0], &thresholds[1]);
                }
                plan->lolo = thresholds[0];
                plan->lo = thresholds[1];
                plan->hi = NAN;
                plan->hihi = NAN;
                break;
        }
        case THRESHOLD_TYPE_DISCRETE_WARN:
                plan->lolo = NAN;
                plan->lo = NAN;
                plan->hi = config->hi;
                plan->hihi = NAN;
                break;
        case THRESHOLD_TYPE_DISCRETE_TRIP:
                plan->lolo = NAN;
                plan->lo = NAN;
                plan->hi = NAN;
                plan->hihi = config->hihi;
                break;
        default: return THRESHOLD_STATUS_INVALID_ARG;
        }

        plan->valid = true;
        return THRESHOLD_STATUS_OK;
}

threshold_severity_t
threshold_plan_eval(const threshold_plan_t *plan, float sample)
{
        if ((plan == NULL) || !plan->valid) {
                return THRESHOLD_SEV_INVALID;
        }

        if (plan->type == THRESHOLD_TYPE_NONE) {
                return THRESHOLD_SEV_OK;
        }

        if (!is_finite(sample)) {
                return apply_invalid_policy(plan->policy);
        }

        switch (plan->type) {
        case THRESHOLD_TYPE_RANGE: return eval_range(plan, sample);
        case THRESHOLD_TYPE_UPPER: return eval_upper(plan, sample);
        case THRESHOLD_TYPE_LOWER: return eval_lower(plan, sample);
        case THRESHOLD_TYPE_DISCRETE_WARN:
                return eval_discrete_warn(plan, sample);
        case THRESHOLD_TYPE_DISCRETE_TRIP:
                return eval_discrete_trip(plan, sample);
        default: return THRESHOLD_SEV_INVALID;
        }
}

threshold_severity_t
threshold_plan_eval_hys(const threshold_plan_t *plan, float sample,
                        float hysteresis, threshold_severity_t prev)
{
        if ((plan == NULL) || !plan->valid) {
                return THRESHOLD_SEV_INVALID;
        }
        if (!is_finite(hysteresis) || (hysteresis < 0.0f)) {
                return THRESHOLD_SEV_INVALID;
        }

        if (plan->type == THRESHOLD_TYPE_NONE) {
                return THRESHOLD_SEV_OK;
        }

        if (!is_finite(sample)) {
                return apply_invalid_policy(plan->policy);
        }

        /* Build hysteresis-adjusted plan copy for evaluation */
        threshold_plan_t adjusted = *plan;

        switch (prev) {
        case THRESHOLD_SEV_TRIP_HIGH:
                if (is_finite(adjusted.hihi)) {
                        adjusted.hihi -= hysteresis;
                }
                break;
        case THRESHOLD_SEV_WARN_HIGH:
                if (is_finite(adjusted.hi)) {
                        adjusted.hi -= hysteresis;
                }
                break;
        case THRESHOLD_SEV_TRIP_LOW:
                if (is_finite(adjusted.lolo)) {
                        adjusted.lolo += hysteresis;
                }
                break;
        case THRESHOLD_SEV_WARN_LOW:
                if (is_finite(adjusted.lo)) {
                        adjusted.lo += hysteresis;
                }
                break;
        default:
                /* No adjustment for OK or INVALID */
                break;
        }

        switch (adjusted.type) {
        case THRESHOLD_TYPE_RANGE: return eval_range(&adjusted, sample);
        case THRESHOLD_TYPE_UPPER: return eval_upper(&adjusted, sample);
        case THRESHOLD_TYPE_LOWER: return eval_lower(&adjusted, sample);
        case THRESHOLD_TYPE_DISCRETE_WARN:
                return eval_discrete_warn(&adjusted, sample);
        case THRESHOLD_TYPE_DISCRETE_TRIP:
                return eval_discrete_trip(&adjusted, sample);
        default: return THRESHOLD_SEV_INVALID;
        }
}

const char *
threshold_severity_name(threshold_severity_t sev)
{
        switch (sev) {
        case THRESHOLD_SEV_OK: return "OK";
        case THRESHOLD_SEV_WARN_LOW: return "WARN_LOW";
        case THRESHOLD_SEV_WARN_HIGH: return "WARN_HIGH";
        case THRESHOLD_SEV_TRIP_LOW: return "TRIP_LOW";
        case THRESHOLD_SEV_TRIP_HIGH: return "TRIP_HIGH";
        case THRESHOLD_SEV_INVALID: return "INVALID";
        default: return "UNKNOWN";
        }
}

const char *
threshold_type_name(threshold_type_t type)
{
        switch (type) {
        case THRESHOLD_TYPE_NONE: return "NONE";
        case THRESHOLD_TYPE_RANGE: return "RANGE";
        case THRESHOLD_TYPE_UPPER: return "UPPER";
        case THRESHOLD_TYPE_LOWER: return "LOWER";
        case THRESHOLD_TYPE_DISCRETE_WARN: return "DISCRETE_WARN";
        case THRESHOLD_TYPE_DISCRETE_TRIP: return "DISCRETE_TRIP";
        default: return "UNKNOWN";
        }
}

const char *
threshold_status_str(threshold_status_t status)
{
        switch (status) {
        case THRESHOLD_STATUS_OK: return "OK";
        case THRESHOLD_STATUS_INVALID_ARG: return "Invalid argument";
        case THRESHOLD_STATUS_OUT_OF_RANGE:
                return "Threshold exceeds maximum range";
        case THRESHOLD_STATUS_MISSING: return "Required threshold is missing";
        case THRESHOLD_STATUS_ORDER:
                return "Thresholds are not in monotonic order";
        case THRESHOLD_STATUS_EXTRA:
                return "Extra thresholds provided for plan type";
        default: return "Unknown status";
        }
}
