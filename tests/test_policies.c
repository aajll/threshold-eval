/**
 * @file test_policies.c
 * @brief Unit tests for policy flag combinations and invalid sample handling.
 */

#include "threshold_eval.h"
#include "test_harness.h"

/* ================ Policy flag combination tests ============================
 */

TEST_CASE(test_policy_flags_combined_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_flags_combined_deescalate_warn)
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

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_flags_combined_ignore_invalid)
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

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_flags_combined_multiple_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP
                     | THRESHOLD_POLICY_DEESCALATE_WARN
                     | THRESHOLD_POLICY_IGNORE_INVALID;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);

        /* FAILSAFE_TRIP has highest priority */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_flags_combined_with_reorder)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f;
        cfg.lo = 80.0f;
        cfg.hi = 20.0f;
        cfg.hihi = 10.0f;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        /* Thresholds should be reordered */
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

/* ================ OPERATIONAL macro test ================================== */

TEST_CASE(test_policy_operational_macro)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_OPERATIONAL;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        /* DEESCALATE_WARN: NaN returns WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

/* ================ STRICT_CONFIG | ALLOW_REORDER interaction =============== */

TEST_CASE(test_strict_config_with_allow_reorder_range)
{
        /* When both STRICT_CONFIG and ALLOW_REORDER are set, reordering
         * takes precedence (ordering check is skipped, thresholds are
         * sorted). */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 90.0f;
        cfg.lo = 80.0f;
        cfg.hi = 20.0f;
        cfg.hihi = 10.0f;
        cfg.policy =
            THRESHOLD_POLICY_STRICT_CONFIG | THRESHOLD_POLICY_ALLOW_REORDER;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        /* Thresholds should be reordered despite STRICT_CONFIG */
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
}

/* ================ Conflicting invalid-sample policy priority ============== */

TEST_CASE(test_policy_failsafe_over_deescalate_warn)
{
        /* FAILSAFE_TRIP > DEESCALATE_WARN when both are set */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy =
            THRESHOLD_POLICY_FAILSAFE_TRIP | THRESHOLD_POLICY_DEESCALATE_WARN;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_deescalate_over_ignore_invalid)
{
        /* DEESCALATE_WARN > IGNORE_INVALID when both are set */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy =
            THRESHOLD_POLICY_DEESCALATE_WARN | THRESHOLD_POLICY_IGNORE_INVALID;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
}

TEST_CASE(test_policy_all_three_conflicting)
{
        /* FAILSAFE_TRIP > DEESCALATE_WARN > IGNORE_INVALID when all three
         * are set. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP
                     | THRESHOLD_POLICY_DEESCALATE_WARN
                     | THRESHOLD_POLICY_IGNORE_INVALID;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);
        TEST_ASSERT_TRUE(plan.valid);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_policies unit tests ===\n\n");

        run_test(test_policy_flags_combined_failsafe_trip,
                 "test_policy_flags_combined_failsafe_trip");
        run_test(test_policy_flags_combined_deescalate_warn,
                 "test_policy_flags_combined_deescalate_warn");
        run_test(test_policy_flags_combined_ignore_invalid,
                 "test_policy_flags_combined_ignore_invalid");
        run_test(test_policy_flags_combined_multiple_invalid,
                 "test_policy_flags_combined_multiple_invalid");
        run_test(test_policy_flags_combined_with_reorder,
                 "test_policy_flags_combined_with_reorder");
        run_test(test_policy_operational_macro,
                 "test_policy_operational_macro");
        run_test(test_strict_config_with_allow_reorder_range,
                 "test_strict_config_with_allow_reorder_range");
        run_test(test_policy_failsafe_over_deescalate_warn,
                 "test_policy_failsafe_over_deescalate_warn");
        run_test(test_policy_deescalate_over_ignore_invalid,
                 "test_policy_deescalate_over_ignore_invalid");
        run_test(test_policy_all_three_conflicting,
                 "test_policy_all_three_conflicting");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
