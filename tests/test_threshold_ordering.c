/**
 * @file test_threshold_ordering.c
 * @brief Unit tests for threshold ordering edge cases.
 */

#include "threshold_eval.h"
#include "test_harness.h"

/* ================ Threshold ordering edge cases =========================== */

TEST_CASE(test_range_equal_thresholds_valid)
{
        /* All thresholds equal should be valid with strict config */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 50.0f;
        cfg.lo = 50.0f;
        cfg.hi = 50.0f;
        cfg.hihi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.hihi);
}

TEST_CASE(test_range_single_pair_equal)
{
        /* Only lolo == lo, hi and hihi different */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 10.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
}

TEST_CASE(test_range_single_pair_equal_hi_hihi)
{
        /* Only hi == hihi, lolo and lo different */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 80.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
}

TEST_CASE(test_range_all_pairs_equal)
{
        /* lolo == lo == hi == hihi */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 50.0f;
        cfg.lo = 50.0f;
        cfg.hi = 50.0f;
        cfg.hihi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
}

TEST_CASE(test_upper_equal_thresholds_valid)
{
        /* hi == hihi should be valid for UPPER type */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 80.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hihi);
}

TEST_CASE(test_lower_equal_thresholds_valid)
{
        /* lolo == lo should be valid for LOWER type */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 10.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lo);
}

TEST_CASE(test_upper_extra_threshold_with_equal_values)
{
        /* UPPER type with extra threshold (lolo) should fail even if values
         * equal */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.lolo = 80.0f; /* equals hi - extra field violation */
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_EXTRA, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_lower_extra_threshold_with_equal_values)
{
        /* LOWER type with extra threshold (hi) should fail even if values equal
         */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 20.0f; /* equals lo - extra field violation */
        cfg.hihi = 30.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_EXTRA, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_range_mixed_ordering_violations)
{
        /* lolo < lo < hi but hi > hihi - partial ordering violation */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 90.0f; /* > hihi */
        cfg.hihi = 80.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_range_only_lolo_lo_out_of_order)
{
        /* Only first pair out of order */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f; /* > lo */
        cfg.lo = 80.0f;
        cfg.hi = 50.0f;
        cfg.hihi = 40.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_range_only_lo_hi_out_of_order)
{
        /* Only middle pair out of order */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 10.0f; /* < hihi */
        cfg.hihi = 20.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_range_only_hi_hihi_out_of_order)
{
        /* Only last pair out of order */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 70.0f; /* < hi */
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_upper_out_of_order_hi_hihi)
{
        /* hi > hihi should fail for UPPER type */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 90.0f;
        cfg.hihi = 80.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_lower_out_of_order_lolo_lo)
{
        /* lolo > lo should fail for LOWER type */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 20.0f;
        cfg.lo = 10.0f;
        cfg.policy = THRESHOLD_POLICY_STRICT_CONFIG;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_ORDER, status);
        TEST_ASSERT_FALSE(plan.valid);
}

TEST_CASE(test_range_reorder_with_equal_values)
{
        /* ALLOW_REORDER with equal values should succeed, values unchanged */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 50.0f;
        cfg.lo = 50.0f;
        cfg.hi = 50.0f;
        cfg.hihi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 50.0f, plan.hihi);
}

TEST_CASE(test_upper_reorder_with_equal_values)
{
        /* ALLOW_REORDER with equal values for UPPER should succeed */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 80.0f;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hihi);
}

TEST_CASE(test_range_reorder_with_partial_equal_values)
{
        /* ALLOW_REORDER with some equal values */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 10.0f; /* equals lolo */
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

TEST_CASE(test_range_reorder_with_out_of_order_equal_pairs)
{
        /* ALLOW_REORDER with out-of-order but equal adjacent pairs */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f; /* > lo */
        cfg.lo = 90.0f;   /* equals lolo */
        cfg.hi = 80.0f;   /* < hihi */
        cfg.hihi = 80.0f; /* equals hi */
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        /* Should be sorted: 80, 80, 90, 90 */
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

/* ================ Evaluation with equal thresholds ======================= */

TEST_CASE(test_range_eval_at_single_threshold)
{
        /* All thresholds equal - evaluation should work correctly */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 10.0f;
        cfg.hi = 10.0f;
        cfg.hihi = 10.0f;
        cfg.epsilon = 0.5f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* Sample at threshold - should be OK (not strictly above/below) */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 10.0f));

        /* Sample at hi+eps boundary - still OK (strict inequality) */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 10.5f));

        /* Sample above hihi+eps - TRIP */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 11.0f));

        /* Sample at lo-eps boundary - still OK (strict inequality) */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 9.5f));

        /* Sample below lolo-eps - TRIP */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 9.0f));
}

TEST_CASE(test_upper_eval_at_equal_thresholds)
{
        /* hi == hihi - evaluate at threshold */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 80.0f;
        cfg.epsilon = 0.5f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* At threshold - OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));

        /* At hi+eps boundary - still OK (strict inequality) */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));

        /* Above hihi+eps - TRIP */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 81.0f));
}

TEST_CASE(test_lower_eval_at_equal_thresholds)
{
        /* lolo == lo - evaluate at threshold */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 20.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 0.5f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* At threshold - OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));

        /* At lo-eps boundary - still OK (strict inequality) */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.5f));

        /* Below lo-eps - TRIP */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 19.0f));
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout,
                "\n=== Running test_threshold_ordering unit tests ===\n\n");

        run_test(test_range_equal_thresholds_valid,
                 "test_range_equal_thresholds_valid");
        run_test(test_range_single_pair_equal, "test_range_single_pair_equal");
        run_test(test_range_single_pair_equal_hi_hihi,
                 "test_range_single_pair_equal_hi_hihi");
        run_test(test_range_all_pairs_equal, "test_range_all_pairs_equal");
        run_test(test_upper_equal_thresholds_valid,
                 "test_upper_equal_thresholds_valid");
        run_test(test_lower_equal_thresholds_valid,
                 "test_lower_equal_thresholds_valid");
        run_test(test_upper_extra_threshold_with_equal_values,
                 "test_upper_extra_threshold_with_equal_values");
        run_test(test_lower_extra_threshold_with_equal_values,
                 "test_lower_extra_threshold_with_equal_values");
        run_test(test_range_mixed_ordering_violations,
                 "test_range_mixed_ordering_violations");
        run_test(test_range_only_lolo_lo_out_of_order,
                 "test_range_only_lolo_lo_out_of_order");
        run_test(test_range_only_lo_hi_out_of_order,
                 "test_range_only_lo_hi_out_of_order");
        run_test(test_range_only_hi_hihi_out_of_order,
                 "test_range_only_hi_hihi_out_of_order");
        run_test(test_upper_out_of_order_hi_hihi,
                 "test_upper_out_of_order_hi_hihi");
        run_test(test_lower_out_of_order_lolo_lo,
                 "test_lower_out_of_order_lolo_lo");
        run_test(test_range_reorder_with_equal_values,
                 "test_range_reorder_with_equal_values");
        run_test(test_upper_reorder_with_equal_values,
                 "test_upper_reorder_with_equal_values");
        run_test(test_range_reorder_with_partial_equal_values,
                 "test_range_reorder_with_partial_equal_values");
        run_test(test_range_reorder_with_out_of_order_equal_pairs,
                 "test_range_reorder_with_out_of_order_equal_pairs");
        run_test(test_range_eval_at_single_threshold,
                 "test_range_eval_at_single_threshold");
        run_test(test_upper_eval_at_equal_thresholds,
                 "test_upper_eval_at_equal_thresholds");
        run_test(test_lower_eval_at_equal_thresholds,
                 "test_lower_eval_at_equal_thresholds");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
