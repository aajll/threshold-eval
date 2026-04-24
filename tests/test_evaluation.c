/**
 * @file test_evaluation.c
 * @brief Unit tests for threshold_plan_eval.
 */

#include "threshold_eval.h"
#include "test_harness.h"

/* ================ threshold_plan_eval tests =============================== */

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

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_evaluation unit tests ===\n\n");

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

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
