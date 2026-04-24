/**
 * @file test_validation.c
 * @brief Unit tests for threshold_plan_build validation.
 */

#include "threshold_eval.h"
#include "test_harness.h"

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

TEST_CASE(test_plan_build_inf_epsilon_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.epsilon = INFINITY;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_nan_epsilon_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.epsilon = NAN;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_plan_build_neg_inf_epsilon_fails)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.epsilon = -INFINITY;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_INVALID_ARG, status);
        TEST_ASSERT_FALSE(plan.valid);
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_validation unit tests ===\n\n");

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
        run_test(test_plan_build_inf_epsilon_fails,
                 "test_plan_build_inf_epsilon_fails");
        run_test(test_plan_build_nan_epsilon_fails,
                 "test_plan_build_nan_epsilon_fails");
        run_test(test_plan_build_neg_inf_epsilon_fails,
                 "test_plan_build_neg_inf_epsilon_fails");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
