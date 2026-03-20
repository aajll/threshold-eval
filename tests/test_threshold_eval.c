/**
 * @file test_threshold_eval.c
 * @brief Unit tests for threshold-eval.
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
                        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__,       \
                                __LINE__, #expr);                              \
                        exit(EXIT_FAILURE);                                    \
                }                                                              \
        } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT((expected) == (actual))
#define TEST_ASSERT_TRUE(expr)              TEST_ASSERT(!!(expr))
#define TEST_ASSERT_FALSE(expr)             TEST_ASSERT(!(expr))
#define TEST_ASSERT_NOT_NULL(ptr)           TEST_ASSERT((ptr) != NULL)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                             \
        TEST_ASSERT(strcmp((expected), (actual)) == 0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                      \
        TEST_ASSERT(fabsf((float)(actual) - (float)(expected)) <= (float)(delta))

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

/* ================ threshold_config_init tests ============================= */

TEST_CASE(test_config_init_sets_defaults)
{
        threshold_config_t cfg;
        memset(&cfg, 0xFF, sizeof(cfg));

        threshold_status_t status = threshold_config_init(&cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_EQUAL(THRESHOLD_TYPE_NONE, cfg.type);
        TEST_ASSERT_EQUAL(THRESHOLD_POLICY_FAILSAFE, cfg.policy);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, THRESHOLD_EVAL_EPSILON, cfg.epsilon);
        TEST_ASSERT_TRUE(isnan(cfg.lolo));
        TEST_ASSERT_TRUE(isnan(cfg.lo));
        TEST_ASSERT_TRUE(isnan(cfg.hi));
        TEST_ASSERT_TRUE(isnan(cfg.hihi));
}

TEST_CASE(test_config_init_null_returns_error)
{
        threshold_status_t status = threshold_config_init(NULL);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
}

/* ================ threshold_plan_build validation tests =================== */

TEST_CASE(test_plan_build_null_plan_returns_error)
{
        threshold_config_t cfg;
        threshold_config_init(&cfg);

        threshold_status_t status = threshold_plan_build(NULL, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
}

TEST_CASE(test_plan_build_null_config_returns_error)
{
        threshold_plan_t plan;

        threshold_status_t status = threshold_plan_build(&plan, NULL);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_type_none_succeeds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_TYPE_NONE, plan.type);
}

TEST_CASE(test_plan_build_range_valid_thresholds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

TEST_CASE(test_plan_build_range_missing_threshold_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        /* hihi left as NAN */
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_MISSING, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_range_bad_order_strict_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f; /* out of order */
        cfg.lo = 80.0f;
        cfg.hi = 20.0f;
        cfg.hihi = 10.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_range_bad_order_reorder_succeeds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f; /* out of order */
        cfg.lo = 80.0f;
        cfg.hi = 20.0f;
        cfg.hihi = 10.0f;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

TEST_CASE(test_plan_build_upper_valid_thresholds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

TEST_CASE(test_plan_build_upper_extra_threshold_strict_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.lolo = 10.0f; /* extra - not valid for UPPER */
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_EXTRA, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_lower_valid_thresholds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);
}

TEST_CASE(test_plan_build_discrete_warn_valid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.hi);
}

TEST_CASE(test_plan_build_discrete_trip_valid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 100.0f, plan.hihi);
}

TEST_CASE(test_plan_build_threshold_out_of_range_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = THRESHOLD_EVAL_MAX + 1.0f;
        cfg.hihi = THRESHOLD_EVAL_MAX + 2.0f;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OUT_OF_RANGE, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_threshold_nan_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = NAN;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_MISSING, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_threshold_inf_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = INFINITY;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_MISSING, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_negative_epsilon_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.epsilon = -0.001f;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
        TEST_ASSERT_FALSE(plan.valid);
}

/* ================ threshold_plan_eval tests ================================ */

TEST_CASE(test_eval_null_plan_returns_invalid)
{
        threshold_severity_t sev = threshold_plan_eval(NULL, 50.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_invalid_plan_returns_invalid)
{
        threshold_plan_t plan;
        plan.valid = false;

        threshold_severity_t sev = threshold_plan_eval(&plan, 50.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_type_none_always_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 0.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, 1000000.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -1000000.0f));
}

TEST_CASE(test_eval_range_ok_in_middle)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
}

TEST_CASE(test_eval_range_warn_low)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 15.0f));
}

TEST_CASE(test_eval_range_warn_high)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 85.0f));
}

TEST_CASE(test_eval_range_trip_low)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 5.0f));
}

TEST_CASE(test_eval_range_trip_high)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 95.0f));
}

TEST_CASE(test_eval_upper_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -1000.0f));
}

TEST_CASE(test_eval_upper_warn_and_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 85.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 95.0f));
}

TEST_CASE(test_eval_lower_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, 1000.0f));
}

TEST_CASE(test_eval_lower_warn_and_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 15.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 5.0f));
}

TEST_CASE(test_eval_discrete_warn)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 40.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 60.0f));
}

TEST_CASE(test_eval_discrete_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 90.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 110.0f));
}

/* ================ Invalid sample policy tests ============================= */

TEST_CASE(test_eval_nan_default_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_eval_nan_failsafe_trip_returns_trip)
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

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_eval_nan_deescalate_warn_returns_warn)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_eval_nan_ignore_returns_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_eval_inf_treated_as_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

/* ================ Hysteresis tests ======================================== */

TEST_CASE(test_eval_hys_null_plan_returns_invalid)
{
        threshold_severity_t sev =
            threshold_plan_eval_hys(NULL, 50.0f, 1.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_hys_negative_hysteresis_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        threshold_severity_t sev =
            threshold_plan_eval_hys(&plan, 50.0f, -1.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_hys_prevents_chatter_high)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* At 81: first eval triggers WARN_HIGH */
        threshold_severity_t sev1 =
            threshold_plan_eval_hys(&plan, 81.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, sev1);

        /* At 79: with 5.0 hysteresis, hi lowered to 75 -> still WARN_HIGH */
        threshold_severity_t sev2 = threshold_plan_eval_hys(
            &plan, 79.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, sev2);

        /* At 74: below adjusted threshold (75) -> clears to OK */
        threshold_severity_t sev3 = threshold_plan_eval_hys(
            &plan, 74.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, sev3);
}

TEST_CASE(test_eval_hys_prevents_chatter_low)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* At 19: triggers WARN_LOW */
        threshold_severity_t sev1 =
            threshold_plan_eval_hys(&plan, 19.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, sev1);

        /* At 21: with 5.0 hysteresis, lo raised to 25 -> still WARN_LOW */
        threshold_severity_t sev2 = threshold_plan_eval_hys(
            &plan, 21.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, sev2);

        /* At 26: above adjusted threshold (25) -> clears to OK */
        threshold_severity_t sev3 = threshold_plan_eval_hys(
            &plan, 26.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, sev3);
}

TEST_CASE(test_eval_hys_zero_hysteresis_same_as_plain_eval)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        float test_values[] = {5.0f, 15.0f, 50.0f, 85.0f, 95.0f};
        for (size_t i = 0; i < sizeof(test_values) / sizeof(test_values[0]);
             i++) {
                threshold_severity_t plain =
                    threshold_plan_eval(&plan, test_values[i]);
                threshold_severity_t hys = threshold_plan_eval_hys(
                    &plan, test_values[i], 0.0f, THRESHOLD_SEV_OK);
                TEST_ASSERT_EQUAL(plain, hys);
        }
}

/* ================ Helper function tests =================================== */

TEST_CASE(test_severity_is_warn)
{
        TEST_ASSERT_FALSE(threshold_severity_is_warn(THRESHOLD_SEV_OK));
        TEST_ASSERT_TRUE(threshold_severity_is_warn(THRESHOLD_SEV_WARN_LOW));
        TEST_ASSERT_TRUE(threshold_severity_is_warn(THRESHOLD_SEV_WARN_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_warn(THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_FALSE(threshold_severity_is_warn(THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_warn(THRESHOLD_SEV_INVALID));
}

TEST_CASE(test_severity_is_trip)
{
        TEST_ASSERT_FALSE(threshold_severity_is_trip(THRESHOLD_SEV_OK));
        TEST_ASSERT_FALSE(threshold_severity_is_trip(THRESHOLD_SEV_WARN_LOW));
        TEST_ASSERT_FALSE(threshold_severity_is_trip(THRESHOLD_SEV_WARN_HIGH));
        TEST_ASSERT_TRUE(threshold_severity_is_trip(THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_TRUE(threshold_severity_is_trip(THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_trip(THRESHOLD_SEV_INVALID));
}

TEST_CASE(test_severity_is_low)
{
        TEST_ASSERT_FALSE(threshold_severity_is_low(THRESHOLD_SEV_OK));
        TEST_ASSERT_TRUE(threshold_severity_is_low(THRESHOLD_SEV_WARN_LOW));
        TEST_ASSERT_FALSE(threshold_severity_is_low(THRESHOLD_SEV_WARN_HIGH));
        TEST_ASSERT_TRUE(threshold_severity_is_low(THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_FALSE(threshold_severity_is_low(THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_low(THRESHOLD_SEV_INVALID));
}

TEST_CASE(test_severity_is_high)
{
        TEST_ASSERT_FALSE(threshold_severity_is_high(THRESHOLD_SEV_OK));
        TEST_ASSERT_FALSE(threshold_severity_is_high(THRESHOLD_SEV_WARN_LOW));
        TEST_ASSERT_TRUE(threshold_severity_is_high(THRESHOLD_SEV_WARN_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_high(THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_TRUE(threshold_severity_is_high(THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_FALSE(threshold_severity_is_high(THRESHOLD_SEV_INVALID));
}

TEST_CASE(test_severity_name_returns_strings)
{
        TEST_ASSERT_EQUAL_STRING("OK",
                                 threshold_severity_name(THRESHOLD_SEV_OK));
        TEST_ASSERT_EQUAL_STRING(
            "WARN_LOW", threshold_severity_name(THRESHOLD_SEV_WARN_LOW));
        TEST_ASSERT_EQUAL_STRING(
            "WARN_HIGH", threshold_severity_name(THRESHOLD_SEV_WARN_HIGH));
        TEST_ASSERT_EQUAL_STRING(
            "TRIP_LOW", threshold_severity_name(THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_EQUAL_STRING(
            "TRIP_HIGH", threshold_severity_name(THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_EQUAL_STRING(
            "INVALID", threshold_severity_name(THRESHOLD_SEV_INVALID));
        TEST_ASSERT_EQUAL_STRING(
            "UNKNOWN", threshold_severity_name((threshold_severity_t)99));
}

TEST_CASE(test_type_name_returns_strings)
{
        TEST_ASSERT_EQUAL_STRING("NONE",
                                 threshold_type_name(THRESHOLD_TYPE_NONE));
        TEST_ASSERT_EQUAL_STRING("RANGE",
                                 threshold_type_name(THRESHOLD_TYPE_RANGE));
        TEST_ASSERT_EQUAL_STRING("UPPER",
                                 threshold_type_name(THRESHOLD_TYPE_UPPER));
        TEST_ASSERT_EQUAL_STRING("LOWER",
                                 threshold_type_name(THRESHOLD_TYPE_LOWER));
        TEST_ASSERT_EQUAL_STRING(
            "DISCRETE_WARN",
            threshold_type_name(THRESHOLD_TYPE_DISCRETE_WARN));
        TEST_ASSERT_EQUAL_STRING(
            "DISCRETE_TRIP",
            threshold_type_name(THRESHOLD_TYPE_DISCRETE_TRIP));
        TEST_ASSERT_EQUAL_STRING("UNKNOWN",
                                 threshold_type_name((threshold_type_t)99));
}

TEST_CASE(test_status_str_returns_strings)
{
        TEST_ASSERT_EQUAL_STRING("OK",
                                 threshold_status_str(THRESHOLD_STATUS_OK));
        TEST_ASSERT_NOT_NULL(
            threshold_status_str(THRESHOLD_STATUS_INVALID_ARG));
        TEST_ASSERT_NOT_NULL(threshold_status_str(THRESHOLD_STATUS_MISSING));
        TEST_ASSERT_NOT_NULL(threshold_status_str((threshold_status_t)-99));
}

/* ================ Epsilon boundary tests ================================== */

TEST_CASE(test_eval_at_boundary_with_epsilon)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Exactly at 80: not above hi+eps (81), so OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));

        /* At 80.5: not above 81, so OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, 80.5f));

        /* At 81.5: above 81, so WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 81.5f));
}

/* ================ Threshold boundary dead-band tests ====================== */
/*
 * These tests exercise the epsilon dead-band at every threshold boundary for
 * all evaluation types.  The comparison rules are strict inequalities:
 *
 *   TRIP_LOW  : sample < lolo - epsilon
 *   WARN_LOW  : sample < lo   - epsilon
 *   WARN_HIGH : sample > hi   + epsilon
 *   TRIP_HIGH : sample > hihi + epsilon
 *
 * Consequently, values inside the dead-band [threshold ± epsilon] are NOT
 * escalated, and values at the exact dead-band edge are still not escalated
 * (strict inequality).  Only values that strictly pass the boundary trigger.
 */

/* Helper: build a RANGE plan (lolo=10, lo=20, hi=80, hihi=90) with a
 * caller-supplied epsilon and no policy flags. */
static void
build_range_eps(threshold_plan_t *plan, float eps)
{
        threshold_config_t cfg;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_RANGE;
        cfg.lolo    = 10.0f;
        cfg.lo      = 20.0f;
        cfg.hi      = 80.0f;
        cfg.hihi    = 90.0f;
        cfg.epsilon = eps;
        cfg.policy  = 0;
        threshold_plan_build(plan, &cfg);
}

/* --- HI dead-band (eps=1.0): effective trigger at 81.0 ------------------- */

TEST_CASE(test_boundary_hi_exact_at_threshold)
{
        /* hi=80, eps=1 → dead-band [80, 81]; 80 > 81 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
}

TEST_CASE(test_boundary_hi_inside_deadband)
{
        /* 80.5 is inside dead-band (80, 81) → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));
}

TEST_CASE(test_boundary_hi_at_deadband_edge)
{
        /* sample == hi + eps = 81.0; 81 > 81 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 81.0f));
}

TEST_CASE(test_boundary_hi_past_deadband)
{
        /* 81.1 > 81 → WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 81.1f));
}

/* --- HIHI dead-band (eps=1.0): effective trip trigger at 91.0 ------------ */

TEST_CASE(test_boundary_hihi_exact_at_threshold)
{
        /* hihi=90, eps=1 → 90 > 91 is false; 90 > 81 is true → WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
}

TEST_CASE(test_boundary_hihi_inside_deadband)
{
        /* 90.5 is inside dead-band (90, 91) → WARN_HIGH, not TRIP */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.5f));
}

TEST_CASE(test_boundary_hihi_at_deadband_edge)
{
        /* sample == hihi + eps = 91.0; 91 > 91 is false → WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 91.0f));
}

TEST_CASE(test_boundary_hihi_past_deadband)
{
        /* 91.1 > 91 → TRIP_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 91.1f));
}

/* --- LO dead-band (eps=1.0): effective warn trigger at 19.0 -------------- */

TEST_CASE(test_boundary_lo_exact_at_threshold)
{
        /* lo=20, eps=1 → dead-band [19, 20]; 20 < 19 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
}

TEST_CASE(test_boundary_lo_inside_deadband)
{
        /* 19.5 is inside dead-band (19, 20) → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.5f));
}

TEST_CASE(test_boundary_lo_at_deadband_edge)
{
        /* sample == lo - eps = 19.0; 19 < 19 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.0f));
}

TEST_CASE(test_boundary_lo_past_deadband)
{
        /* 18.9 < 19 → WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 18.9f));
}

/* --- LOLO dead-band (eps=1.0): effective trip trigger at 9.0 ------------- */

TEST_CASE(test_boundary_lolo_exact_at_threshold)
{
        /* lolo=10, eps=1 → 10 < 9 is false; 10 < 19 is true → WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
}

TEST_CASE(test_boundary_lolo_inside_deadband)
{
        /* 9.5 is inside dead-band (9, 10) → WARN_LOW, not TRIP */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.5f));
}

TEST_CASE(test_boundary_lolo_at_deadband_edge)
{
        /* sample == lolo - eps = 9.0; 9 < 9 is false → WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.0f));
}

TEST_CASE(test_boundary_lolo_past_deadband)
{
        /* 8.9 < 9 → TRIP_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 8.9f));
}

/* --- Zero epsilon: no dead-band; strict inequality fires immediately ------ */

TEST_CASE(test_boundary_zero_eps_hi_exact)
{
        /* eps=0: 80 > 80 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
}

TEST_CASE(test_boundary_zero_eps_hi_tiny_above)
{
        /* eps=0: 80.01 > 80 → WARN_HIGH with no dead-band */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 80.01f));
}

TEST_CASE(test_boundary_zero_eps_hihi_exact)
{
        /* eps=0: 90 > 90 is false; 90 > 80 is true → WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
}

TEST_CASE(test_boundary_zero_eps_hihi_tiny_above)
{
        /* eps=0: 90.01 > 90 → TRIP_HIGH immediately */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 90.01f));
}

TEST_CASE(test_boundary_zero_eps_lo_exact)
{
        /* eps=0: 20 < 20 is false → OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
}

TEST_CASE(test_boundary_zero_eps_lo_tiny_below)
{
        /* eps=0: 19.99 < 20 → WARN_LOW with no dead-band */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 19.99f));
}

TEST_CASE(test_boundary_zero_eps_lolo_exact)
{
        /* eps=0: 10 < 10 is false; 10 < 20 is true → WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
}

TEST_CASE(test_boundary_zero_eps_lolo_tiny_below)
{
        /* eps=0: 9.99 < 10 → TRIP_LOW immediately */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 9.99f));
}

/* --- Default epsilon (THRESHOLD_EVAL_EPSILON = 1e-3f) -------------------- */

TEST_CASE(test_boundary_default_eps_hi_boundary)
{
        /* Using the default epsilon from threshold_config_init().
         * hi+eps = 80.0 + 0.001 = 80.001.  Values at 80, at 80+eps/2, and at
         * the edge 80+eps stay OK; a value at 80+eps*2 must trigger. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type   = THRESHOLD_TYPE_UPPER;
        cfg.hi     = 80.0f;
        cfg.hihi   = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan,
                                             80.0f + THRESHOLD_EVAL_EPSILON * 0.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan,
                                             80.0f + THRESHOLD_EVAL_EPSILON));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan,
                                             80.0f + THRESHOLD_EVAL_EPSILON * 2.0f));
}

TEST_CASE(test_boundary_default_eps_lo_boundary)
{
        /* lo-eps = 20.0 - 0.001 = 19.999.  Values at 20, at 20-eps/2, and at
         * the edge 20-eps stay OK; a value at 20-eps*2 must trigger WARN_LOW. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type   = THRESHOLD_TYPE_LOWER;
        cfg.lolo   = 10.0f;
        cfg.lo     = 20.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan,
                                             20.0f - THRESHOLD_EVAL_EPSILON * 0.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan,
                                             20.0f - THRESHOLD_EVAL_EPSILON));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan,
                                             20.0f - THRESHOLD_EVAL_EPSILON * 2.0f));
}

/* --- Per-type boundary tests: LOWER type --------------------------------- */

TEST_CASE(test_boundary_lower_type_all_boundaries)
{
        /* Verify epsilon dead-band on both LOLO and LO for LOWER type. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_LOWER;
        cfg.lolo    = 10.0f;
        cfg.lo      = 20.0f;
        cfg.epsilon = 1.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* LO boundary: dead-band [19, 20] → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.0f));
        /* Just past LO dead-band → WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 18.9f));
        /* LOLO boundary: dead-band [9, 10] → WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.0f));
        /* Just past LOLO dead-band → TRIP_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 8.9f));
}

/* --- Per-type boundary tests: UPPER type --------------------------------- */

TEST_CASE(test_boundary_upper_type_all_boundaries)
{
        /* Verify epsilon dead-band on both HI and HIHI for UPPER type. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_UPPER;
        cfg.hi      = 80.0f;
        cfg.hihi    = 90.0f;
        cfg.epsilon = 1.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* HI boundary: dead-band [80, 81] → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 81.0f));
        /* Just past HI dead-band → WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 81.1f));
        /* HIHI boundary: dead-band [90, 91] → WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 91.0f));
        /* Just past HIHI dead-band → TRIP_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 91.1f));
}

/* --- Per-type boundary tests: DISCRETE_WARN ------------------------------ */

TEST_CASE(test_boundary_discrete_warn_eps)
{
        /* DISCRETE_WARN: hi=50, eps=2 → dead-band [50, 52]. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi      = 50.0f;
        cfg.epsilon = 2.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* At threshold → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        /* Inside dead-band → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 51.0f));
        /* At dead-band edge hi+eps=52; 52 > 52 is false → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 52.0f));
        /* Past dead-band → WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 52.1f));
}

/* --- Per-type boundary tests: DISCRETE_TRIP ------------------------------ */

TEST_CASE(test_boundary_discrete_trip_eps)
{
        /* DISCRETE_TRIP: hihi=100, eps=2 → dead-band [100, 102]. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi    = 100.0f;
        cfg.epsilon = 2.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* At threshold → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 100.0f));
        /* Inside dead-band → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 101.0f));
        /* At dead-band edge hihi+eps=102; 102 > 102 is false → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 102.0f));
        /* Past dead-band → TRIP_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 102.1f));
}

/* --- Negative threshold values with epsilon ------------------------------ */

TEST_CASE(test_boundary_negative_thresholds_eps)
{
        /* All thresholds in the negative range: lolo=-90, lo=-80,
         * hi=-20, hihi=-10, eps=1.  Verify dead-bands work correctly
         * for negative values. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_RANGE;
        cfg.lolo    = -90.0f;
        cfg.lo      = -80.0f;
        cfg.hi      = -20.0f;
        cfg.hihi    = -10.0f;
        cfg.epsilon = 1.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* Middle → OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -50.0f));

        /* HI dead-band [-20, -19]: hi+eps = -20+1 = -19 */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -20.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -19.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -19.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -18.9f));

        /* HIHI dead-band [-10, -9]: hihi+eps = -10+1 = -9 */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -10.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -9.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -8.9f));

        /* LO dead-band [-81, -80]: lo-eps = -80-1 = -81 */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -80.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -80.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -81.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, -81.1f));

        /* LOLO dead-band [-91, -90]: lolo-eps = -90-1 = -91 */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, -90.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, -91.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, -91.1f));
}

/* --- Hysteresis + epsilon interaction ------------------------------------ */

TEST_CASE(test_boundary_hys_eps_high)
{
        /* With prev=WARN_HIGH and hysteresis=3, the hi threshold is lowered
         * to hi-hys = 80-3 = 77.  Epsilon then applies: trigger at 77+1=78.
         *
         * Without hysteresis the trigger would be at hi+eps = 81.  This test
         * confirms the hysteresis-adjusted threshold still respects epsilon.
         */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_UPPER;
        cfg.hi      = 80.0f;
        cfg.hihi    = 95.0f;
        cfg.epsilon = 1.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* First crossing: 82 > hi+eps=81 → WARN_HIGH */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 82.0f, 3.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* Drops to 79: without hys would clear (79 < 81), but with hys the
         * effective trigger is 78, so 79 > 78 → stays WARN_HIGH */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 79.0f, 3.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Drops to 78: at the dead-band edge; 78 > 78 is false → clears OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 78.0f, 3.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

TEST_CASE(test_boundary_hys_eps_low)
{
        /* With prev=WARN_LOW and hysteresis=3, the lo threshold is raised to
         * lo+hys = 20+3 = 23.  Epsilon applies: trigger at 23-1=22.
         *
         * Values between 22 and 23 stay WARN_LOW due to hysteresis+epsilon.
         */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type    = THRESHOLD_TYPE_LOWER;
        cfg.lolo    = 5.0f;
        cfg.lo      = 20.0f;
        cfg.epsilon = 1.0f;
        cfg.policy  = 0;
        threshold_plan_build(&plan, &cfg);

        /* First crossing: 18 < lo-eps=19 → WARN_LOW */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 18.0f, 3.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s1);

        /* Rises to 21: without hys would clear (21 >= 19), but with hys the
         * effective trigger is 22, so 21 < 22 → stays WARN_LOW */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 21.0f, 3.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s2);

        /* Rises to 22: at the dead-band edge; 22 < 22 is false → clears OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 22.0f, 3.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running threshold_eval unit tests ===\n\n");

        /* Config init tests */
        run_test(test_config_init_sets_defaults,
                 "test_config_init_sets_defaults");
        run_test(test_config_init_null_returns_error,
                 "test_config_init_null_returns_error");

        /* Plan build validation tests */
        run_test(test_plan_build_null_plan_returns_error,
                 "test_plan_build_null_plan_returns_error");
        run_test(test_plan_build_null_config_returns_error,
                 "test_plan_build_null_config_returns_error");
        run_test(test_plan_build_type_none_succeeds,
                 "test_plan_build_type_none_succeeds");
        run_test(test_plan_build_range_valid_thresholds,
                 "test_plan_build_range_valid_thresholds");
        run_test(test_plan_build_range_missing_threshold_fails,
                 "test_plan_build_range_missing_threshold_fails");
        run_test(test_plan_build_range_bad_order_strict_fails,
                 "test_plan_build_range_bad_order_strict_fails");
        run_test(test_plan_build_range_bad_order_reorder_succeeds,
                 "test_plan_build_range_bad_order_reorder_succeeds");
        run_test(test_plan_build_upper_valid_thresholds,
                 "test_plan_build_upper_valid_thresholds");
        run_test(test_plan_build_upper_extra_threshold_strict_fails,
                 "test_plan_build_upper_extra_threshold_strict_fails");
        run_test(test_plan_build_lower_valid_thresholds,
                 "test_plan_build_lower_valid_thresholds");
        run_test(test_plan_build_discrete_warn_valid,
                 "test_plan_build_discrete_warn_valid");
        run_test(test_plan_build_discrete_trip_valid,
                 "test_plan_build_discrete_trip_valid");
        run_test(test_plan_build_threshold_out_of_range_fails,
                 "test_plan_build_threshold_out_of_range_fails");
        run_test(test_plan_build_threshold_nan_fails,
                 "test_plan_build_threshold_nan_fails");
        run_test(test_plan_build_threshold_inf_fails,
                 "test_plan_build_threshold_inf_fails");
        run_test(test_plan_build_negative_epsilon_fails,
                 "test_plan_build_negative_epsilon_fails");

        /* Evaluation tests */
        run_test(test_eval_null_plan_returns_invalid,
                 "test_eval_null_plan_returns_invalid");
        run_test(test_eval_invalid_plan_returns_invalid,
                 "test_eval_invalid_plan_returns_invalid");
        run_test(test_eval_type_none_always_ok,
                 "test_eval_type_none_always_ok");
        run_test(test_eval_range_ok_in_middle, "test_eval_range_ok_in_middle");
        run_test(test_eval_range_warn_low, "test_eval_range_warn_low");
        run_test(test_eval_range_warn_high, "test_eval_range_warn_high");
        run_test(test_eval_range_trip_low, "test_eval_range_trip_low");
        run_test(test_eval_range_trip_high, "test_eval_range_trip_high");
        run_test(test_eval_upper_ok, "test_eval_upper_ok");
        run_test(test_eval_upper_warn_and_trip,
                 "test_eval_upper_warn_and_trip");
        run_test(test_eval_lower_ok, "test_eval_lower_ok");
        run_test(test_eval_lower_warn_and_trip,
                 "test_eval_lower_warn_and_trip");
        run_test(test_eval_discrete_warn, "test_eval_discrete_warn");
        run_test(test_eval_discrete_trip, "test_eval_discrete_trip");

        /* Invalid sample policy tests */
        run_test(test_eval_nan_default_returns_invalid,
                 "test_eval_nan_default_returns_invalid");
        run_test(test_eval_nan_failsafe_trip_returns_trip,
                 "test_eval_nan_failsafe_trip_returns_trip");
        run_test(test_eval_nan_deescalate_warn_returns_warn,
                 "test_eval_nan_deescalate_warn_returns_warn");
        run_test(test_eval_nan_ignore_returns_ok,
                 "test_eval_nan_ignore_returns_ok");
        run_test(test_eval_inf_treated_as_invalid,
                 "test_eval_inf_treated_as_invalid");

        /* Hysteresis tests */
        run_test(test_eval_hys_null_plan_returns_invalid,
                 "test_eval_hys_null_plan_returns_invalid");
        run_test(test_eval_hys_negative_hysteresis_returns_invalid,
                 "test_eval_hys_negative_hysteresis_returns_invalid");
        run_test(test_eval_hys_prevents_chatter_high,
                 "test_eval_hys_prevents_chatter_high");
        run_test(test_eval_hys_prevents_chatter_low,
                 "test_eval_hys_prevents_chatter_low");
        run_test(test_eval_hys_zero_hysteresis_same_as_plain_eval,
                 "test_eval_hys_zero_hysteresis_same_as_plain_eval");

        /* Helper function tests */
        run_test(test_severity_is_warn, "test_severity_is_warn");
        run_test(test_severity_is_trip, "test_severity_is_trip");
        run_test(test_severity_is_low, "test_severity_is_low");
        run_test(test_severity_is_high, "test_severity_is_high");
        run_test(test_severity_name_returns_strings,
                 "test_severity_name_returns_strings");
        run_test(test_type_name_returns_strings,
                 "test_type_name_returns_strings");
        run_test(test_status_str_returns_strings,
                 "test_status_str_returns_strings");

        /* Epsilon boundary tests */
        run_test(test_eval_at_boundary_with_epsilon,
                 "test_eval_at_boundary_with_epsilon");

        /* Threshold boundary dead-band tests */
        run_test(test_boundary_hi_exact_at_threshold,
                 "test_boundary_hi_exact_at_threshold");
        run_test(test_boundary_hi_inside_deadband,
                 "test_boundary_hi_inside_deadband");
        run_test(test_boundary_hi_at_deadband_edge,
                 "test_boundary_hi_at_deadband_edge");
        run_test(test_boundary_hi_past_deadband,
                 "test_boundary_hi_past_deadband");
        run_test(test_boundary_hihi_exact_at_threshold,
                 "test_boundary_hihi_exact_at_threshold");
        run_test(test_boundary_hihi_inside_deadband,
                 "test_boundary_hihi_inside_deadband");
        run_test(test_boundary_hihi_at_deadband_edge,
                 "test_boundary_hihi_at_deadband_edge");
        run_test(test_boundary_hihi_past_deadband,
                 "test_boundary_hihi_past_deadband");
        run_test(test_boundary_lo_exact_at_threshold,
                 "test_boundary_lo_exact_at_threshold");
        run_test(test_boundary_lo_inside_deadband,
                 "test_boundary_lo_inside_deadband");
        run_test(test_boundary_lo_at_deadband_edge,
                 "test_boundary_lo_at_deadband_edge");
        run_test(test_boundary_lo_past_deadband, "test_boundary_lo_past_deadband");
        run_test(test_boundary_lolo_exact_at_threshold,
                 "test_boundary_lolo_exact_at_threshold");
        run_test(test_boundary_lolo_inside_deadband,
                 "test_boundary_lolo_inside_deadband");
        run_test(test_boundary_lolo_at_deadband_edge,
                 "test_boundary_lolo_at_deadband_edge");
        run_test(test_boundary_lolo_past_deadband,
                 "test_boundary_lolo_past_deadband");
        run_test(test_boundary_zero_eps_hi_exact,
                 "test_boundary_zero_eps_hi_exact");
        run_test(test_boundary_zero_eps_hi_tiny_above,
                 "test_boundary_zero_eps_hi_tiny_above");
        run_test(test_boundary_zero_eps_hihi_exact,
                 "test_boundary_zero_eps_hihi_exact");
        run_test(test_boundary_zero_eps_hihi_tiny_above,
                 "test_boundary_zero_eps_hihi_tiny_above");
        run_test(test_boundary_zero_eps_lo_exact,
                 "test_boundary_zero_eps_lo_exact");
        run_test(test_boundary_zero_eps_lo_tiny_below,
                 "test_boundary_zero_eps_lo_tiny_below");
        run_test(test_boundary_zero_eps_lolo_exact,
                 "test_boundary_zero_eps_lolo_exact");
        run_test(test_boundary_zero_eps_lolo_tiny_below,
                 "test_boundary_zero_eps_lolo_tiny_below");
        run_test(test_boundary_default_eps_hi_boundary,
                 "test_boundary_default_eps_hi_boundary");
        run_test(test_boundary_default_eps_lo_boundary,
                 "test_boundary_default_eps_lo_boundary");
        run_test(test_boundary_lower_type_all_boundaries,
                 "test_boundary_lower_type_all_boundaries");
        run_test(test_boundary_upper_type_all_boundaries,
                 "test_boundary_upper_type_all_boundaries");
        run_test(test_boundary_discrete_warn_eps,
                 "test_boundary_discrete_warn_eps");
        run_test(test_boundary_discrete_trip_eps,
                 "test_boundary_discrete_trip_eps");
        run_test(test_boundary_negative_thresholds_eps,
                 "test_boundary_negative_thresholds_eps");
        run_test(test_boundary_hys_eps_high, "test_boundary_hys_eps_high");
        run_test(test_boundary_hys_eps_low, "test_boundary_hys_eps_low");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
