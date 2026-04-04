/**
 * @file test_edge_cases.c
 * @brief Unit tests for edge cases identified during pre-release audit.
 *
 * Covers: invalid enum types, extreme hysteresis, NONE type with invalid
 * input, all-negative thresholds with epsilon, extreme epsilon values,
 * plan rebuild after failure, and DEESCALATE_WARN across threshold types.
 */

#include "threshold_eval.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================ Test framework ========================================== */

#define TEST_ASSERT(expr)                                                      \
        do {                                                                   \
                if (!(expr)) {                                                 \
                        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__,         \
                                __LINE__, #expr);                              \
                        exit(EXIT_FAILURE);                                    \
                }                                                              \
        } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT((expected) == (actual))
#define TEST_ASSERT_TRUE(expr)              TEST_ASSERT(!!(expr))
#define TEST_ASSERT_FALSE(expr)             TEST_ASSERT(!(expr))

#define TEST_PASS(name) fprintf(stdout, "PASS  %s\n", (name))

#define TEST_CASE(name)                                                        \
        static void name(void);                                                \
        static void name(void)

static void
run_test(void (*test_func)(void), const char *name)
{
        test_func();
        TEST_PASS(name);
}

/* ================ Invalid enum type tests ================================= */

/**
 * @brief plan_build rejects an out-of-range type enum.
 */
TEST_CASE(test_plan_build_invalid_type_returns_invalid_arg)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = (threshold_type_t)99;

        threshold_status_t st = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, st);
        TEST_ASSERT_FALSE(plan.valid);
}

/**
 * @brief plan_eval returns INVALID when plan type is corrupted after build.
 *
 * Simulates memory corruption or an uninitialised plan by setting the type
 * field to an invalid value after a successful build.
 */
TEST_CASE(test_plan_eval_invalid_type_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        threshold_plan_build(&plan, &cfg);

        /* Corrupt the type field */
        plan.type = (threshold_type_t)99;

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, 50.0f));
}

/**
 * @brief plan_eval_hys returns INVALID when plan type is corrupted.
 */
TEST_CASE(test_plan_eval_hys_invalid_type_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        threshold_plan_build(&plan, &cfg);

        plan.type = (threshold_type_t)99;

        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_INVALID,
            threshold_plan_eval_hys(&plan, 50.0f, 1.0f, THRESHOLD_SEV_OK));
}

/* ================ Extreme hysteresis tests ================================ */

/**
 * @brief Hysteresis larger than the threshold gap inverts hi below lo.
 *
 * With hi=80, prev=WARN_HIGH, hysteresis=100, the adjusted hi becomes -20.
 * A sample at 50 (above lo=20) should now trigger WARN_HIGH because the
 * adjusted hi is far below the sample.
 */
TEST_CASE(test_hys_exceeding_threshold_gap_range)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 0.0f;
        threshold_plan_build(&plan, &cfg);

        /*
         * prev=WARN_HIGH => adjusted.hi = 80 - 100 = -20.
         * A sample of 50 is above adjusted hi (-20), so severity escalates.
         */
        threshold_severity_t sev = threshold_plan_eval_hys(
            &plan, 50.0f, 100.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_TRUE(sev == THRESHOLD_SEV_WARN_HIGH
                         || sev == THRESHOLD_SEV_TRIP_HIGH);
}

/**
 * @brief Hysteresis larger than the threshold value for TRIP_HIGH.
 *
 * hihi=90, hysteresis=200 => adjusted hihi = -110.
 * Sample at 50 is above the adjusted hihi, triggering TRIP_HIGH.
 */
TEST_CASE(test_hys_exceeding_threshold_value_trip_high)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 0.0f;
        threshold_plan_build(&plan, &cfg);

        threshold_severity_t sev = threshold_plan_eval_hys(
            &plan, 50.0f, 200.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, sev);
}

/**
 * @brief Hysteresis larger than the threshold value for TRIP_LOW.
 *
 * lolo=10, hysteresis=200 => adjusted lolo = 210.
 * Sample at 50 is below the adjusted lolo, triggering TRIP_LOW.
 */
TEST_CASE(test_hys_exceeding_threshold_value_trip_low)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 0.0f;
        threshold_plan_build(&plan, &cfg);

        threshold_severity_t sev = threshold_plan_eval_hys(
            &plan, 50.0f, 200.0f, THRESHOLD_SEV_TRIP_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW, sev);
}

/* ================ NONE type with invalid input =============================
 */

/**
 * @brief TYPE_NONE returns OK for NaN regardless of policy.
 *
 * The NONE early-return fires before the non-finite check.
 */
TEST_CASE(test_type_none_nan_returns_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
}

/**
 * @brief TYPE_NONE returns OK for Inf regardless of policy.
 */
TEST_CASE(test_type_none_inf_returns_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -INFINITY));
}

/**
 * @brief TYPE_NONE with hysteresis still returns OK for NaN.
 */
TEST_CASE(test_type_none_hys_nan_returns_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval_hys(&plan, NAN, 1.0f, THRESHOLD_SEV_OK));
}

/* ================ All-negative thresholds with epsilon ==================== */

/**
 * @brief All-negative RANGE thresholds with default epsilon.
 *
 * Verifies epsilon arithmetic works correctly when thresholds are negative
 * (subtracting epsilon from a negative lolo makes it more negative).
 */
TEST_CASE(test_all_negative_range_with_epsilon)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = -90.0f;
        cfg.lo = -80.0f;
        cfg.hi = -20.0f;
        cfg.hihi = -10.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Well inside OK band */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -50.0f));

        /* Just inside hi dead-band: sample > hi but within epsilon */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -19.5f));

        /* Past hi + epsilon: should be WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -18.0f));

        /* Just inside lo dead-band: sample < lo but within epsilon */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -80.5f));

        /* Past lo - epsilon: should be WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, -82.0f));

        /* Past hihi + epsilon: TRIP_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -8.0f));

        /* Past lolo - epsilon: TRIP_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, -92.0f));
}

/**
 * @brief All-negative LOWER thresholds with epsilon.
 */
TEST_CASE(test_all_negative_lower_with_epsilon)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = -100.0f;
        cfg.lo = -50.0f;
        cfg.epsilon = 2.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Above lo: OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -40.0f));

        /* Inside lo dead-band */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -51.0f));

        /* Below lo - epsilon: WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, -55.0f));

        /* Below lolo - epsilon: TRIP_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, -105.0f));
}

/* ================ Extreme epsilon tests =================================== */

/**
 * @brief Epsilon larger than the threshold span collapses detection.
 *
 * With hi=80, hihi=90 and epsilon=200, the dead-bands extend well past the
 * thresholds. A sample must be > hi+eps (280) to trigger WARN_HIGH.
 */
TEST_CASE(test_large_epsilon_suppresses_all_transitions)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 200.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Values that would normally trigger, but epsilon absorbs them */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 5.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 95.0f));

        /* Far enough past threshold + epsilon to trigger */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 300.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, -200.0f));
}

/**
 * @brief Very small epsilon near floating-point precision.
 */
TEST_CASE(test_tiny_epsilon_near_fp_precision)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 100.0f;
        cfg.hihi = 200.0f;
        cfg.epsilon = 1.0e-7f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Well below: OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 99.0f));

        /* Well above hi + epsilon: WARN */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 101.0f));
}

/* ================ Plan rebuild after failure ===============================
 */

/**
 * @brief A plan that failed to build can be successfully rebuilt with valid
 *        configuration.
 */
TEST_CASE(test_plan_rebuild_after_failure)
{
        threshold_config_t cfg;
        threshold_plan_t plan;

        /* First build: intentionally fail (missing thresholds) */
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        /* lolo/lo/hi/hihi are all NAN from init => MISSING */
        threshold_status_t st = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_MISSING, st);
        TEST_ASSERT_FALSE(plan.valid);

        /* Eval on failed plan returns INVALID */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, 50.0f));

        /* Second build: valid configuration on the same plan struct */
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        st = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, st);
        TEST_ASSERT_TRUE(plan.valid);

        /* Eval now works correctly */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 95.0f));
}

/**
 * @brief A valid plan overwritten by a failed build is no longer usable.
 */
TEST_CASE(test_valid_plan_invalidated_by_failed_rebuild)
{
        threshold_config_t cfg;
        threshold_plan_t plan;

        /* First build: success */
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_TRUE(plan.valid);

        /* Second build: fail (out of order, strict) */
        cfg.lolo = 90.0f;
        cfg.hihi = 10.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;
        threshold_status_t st = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_TRUE(st < 0);
        TEST_ASSERT_FALSE(plan.valid);

        /* Plan is now invalid */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, 50.0f));
}

/* ================ DEESCALATE_WARN across threshold types ================== */

/**
 * @brief DEESCALATE_WARN with UPPER type returns WARN_HIGH for NaN.
 */
TEST_CASE(test_deescalate_warn_upper_nan)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

/**
 * @brief DEESCALATE_WARN with LOWER type returns WARN_HIGH for NaN.
 *
 * Note: the policy always returns WARN_HIGH regardless of type, because the
 * policy resolution is type-agnostic.
 */
TEST_CASE(test_deescalate_warn_lower_nan)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

/**
 * @brief DEESCALATE_WARN with DISCRETE_WARN type.
 */
TEST_CASE(test_deescalate_warn_discrete_warn_nan)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

/**
 * @brief DEESCALATE_WARN with DISCRETE_TRIP type.
 */
TEST_CASE(test_deescalate_warn_discrete_trip_nan)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

/* ================ Hysteresis with NaN samples ============================= */

/**
 * @brief Hysteresis eval with NaN sample applies policy, not hysteresis.
 */
TEST_CASE(test_hys_nan_sample_applies_policy)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        /* NaN sample with non-trivial prev and hysteresis */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_TRIP_HIGH,
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_WARN_HIGH));
}

/**
 * @brief Hysteresis eval with Inf sample and IGNORE_INVALID returns OK.
 */
TEST_CASE(test_hys_inf_sample_ignore_policy)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval_hys(&plan, INFINITY, 2.0f,
                                                  THRESHOLD_SEV_TRIP_HIGH));
}

/* ================ Partially sorted with ALLOW_REORDER ==================== */

/**
 * @brief RANGE with only hi/hihi swapped (lolo/lo already sorted).
 */
TEST_CASE(test_reorder_partially_sorted_range)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 90.0f;   /* swapped */
        cfg.hihi = 80.0f; /* swapped */
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_TRUE(plan.valid);
        /* After reorder: hi=80, hihi=90 */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 85.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 95.0f));
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_edge_cases unit tests ===\n\n");

        /* Invalid enum types */
        run_test(test_plan_build_invalid_type_returns_invalid_arg,
                 "test_plan_build_invalid_type_returns_invalid_arg");
        run_test(test_plan_eval_invalid_type_returns_invalid,
                 "test_plan_eval_invalid_type_returns_invalid");
        run_test(test_plan_eval_hys_invalid_type_returns_invalid,
                 "test_plan_eval_hys_invalid_type_returns_invalid");

        /* Extreme hysteresis */
        run_test(test_hys_exceeding_threshold_gap_range,
                 "test_hys_exceeding_threshold_gap_range");
        run_test(test_hys_exceeding_threshold_value_trip_high,
                 "test_hys_exceeding_threshold_value_trip_high");
        run_test(test_hys_exceeding_threshold_value_trip_low,
                 "test_hys_exceeding_threshold_value_trip_low");

        /* NONE type with invalid input */
        run_test(test_type_none_nan_returns_ok,
                 "test_type_none_nan_returns_ok");
        run_test(test_type_none_inf_returns_ok,
                 "test_type_none_inf_returns_ok");
        run_test(test_type_none_hys_nan_returns_ok,
                 "test_type_none_hys_nan_returns_ok");

        /* All-negative thresholds with epsilon */
        run_test(test_all_negative_range_with_epsilon,
                 "test_all_negative_range_with_epsilon");
        run_test(test_all_negative_lower_with_epsilon,
                 "test_all_negative_lower_with_epsilon");

        /* Extreme epsilon */
        run_test(test_large_epsilon_suppresses_all_transitions,
                 "test_large_epsilon_suppresses_all_transitions");
        run_test(test_tiny_epsilon_near_fp_precision,
                 "test_tiny_epsilon_near_fp_precision");

        /* Plan rebuild after failure */
        run_test(test_plan_rebuild_after_failure,
                 "test_plan_rebuild_after_failure");
        run_test(test_valid_plan_invalidated_by_failed_rebuild,
                 "test_valid_plan_invalidated_by_failed_rebuild");

        /* DEESCALATE_WARN across types */
        run_test(test_deescalate_warn_upper_nan,
                 "test_deescalate_warn_upper_nan");
        run_test(test_deescalate_warn_lower_nan,
                 "test_deescalate_warn_lower_nan");
        run_test(test_deescalate_warn_discrete_warn_nan,
                 "test_deescalate_warn_discrete_warn_nan");
        run_test(test_deescalate_warn_discrete_trip_nan,
                 "test_deescalate_warn_discrete_trip_nan");

        /* Hysteresis with NaN/Inf samples */
        run_test(test_hys_nan_sample_applies_policy,
                 "test_hys_nan_sample_applies_policy");
        run_test(test_hys_inf_sample_ignore_policy,
                 "test_hys_inf_sample_ignore_policy");

        /* Partially sorted reorder */
        run_test(test_reorder_partially_sorted_range,
                 "test_reorder_partially_sorted_range");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
