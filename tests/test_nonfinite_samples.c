/**
 * @file test_nonfinite_samples.c
 * @brief Unit tests for non-finite sample handling across all plan types.
 *
 * Covers: NaN and Inf samples for DISCRETE_WARN, DISCRETE_TRIP, and LOWER
 * types under all three invalid-sample policies (FAILSAFE_TRIP,
 * DEESCALATE_WARN, IGNORE_INVALID), plus hysteresis with NaN and TRIP_LOW
 * prev.
 */

#include "threshold_eval.h"
#include "test_harness.h"

/* ================ NaN samples: DISCRETE_WARN ============================== */

TEST_CASE(test_nan_discrete_warn_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_discrete_warn_deescalate_warn)
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

TEST_CASE(test_nan_discrete_warn_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_discrete_warn_default_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, NAN));
}

/* ================ NaN samples: DISCRETE_TRIP ============================== */

TEST_CASE(test_nan_discrete_trip_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_discrete_trip_deescalate_warn)
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

TEST_CASE(test_nan_discrete_trip_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_discrete_trip_default_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, NAN));
}

/* ================ NaN samples: LOWER type ================================= */

TEST_CASE(test_nan_lower_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_lower_deescalate_warn)
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

TEST_CASE(test_nan_lower_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, NAN));
}

TEST_CASE(test_nan_lower_default_returns_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID,
                          threshold_plan_eval(&plan, NAN));
}

/* ================ Inf samples: DISCRETE_WARN ============================== */

TEST_CASE(test_inf_discrete_warn_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_discrete_warn_deescalate_warn)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_discrete_warn_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -INFINITY));
}

/* ================ Inf samples: DISCRETE_TRIP ============================== */

TEST_CASE(test_inf_discrete_trip_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_discrete_trip_deescalate_warn)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_DEESCALATE_WARN;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_discrete_trip_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -INFINITY));
}

/* ================ Inf samples: LOWER type ================================= */

TEST_CASE(test_inf_lower_failsafe_trip)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_lower_deescalate_warn)
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
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, -INFINITY));
}

TEST_CASE(test_inf_lower_ignore_invalid)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_IGNORE_INVALID;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, INFINITY));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -INFINITY));
}

/* ================ Hysteresis NaN with TRIP_LOW prev ======================= */

TEST_CASE(test_hys_nan_trip_low_prev_range)
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

        /* NaN sample with TRIP_LOW prev: policy applies, not hysteresis */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_TRIP_HIGH,
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_TRIP_LOW));
}

TEST_CASE(test_hys_nan_trip_low_prev_lower)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = THRESHOLD_POLICY_FAILSAFE_TRIP;
        threshold_plan_build(&plan, &cfg);

        /* NaN sample with TRIP_LOW prev: policy applies */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_TRIP_HIGH,
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_TRIP_LOW));
}

TEST_CASE(test_hys_nan_trip_low_prev_deescalate)
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

        /* NaN sample with TRIP_LOW prev: DEESCALATE_WARN wins */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_WARN_HIGH,
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_TRIP_LOW));
}

TEST_CASE(test_hys_nan_trip_low_prev_ignore)
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

        /* NaN sample with TRIP_LOW prev: IGNORE_INVALID wins */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_TRIP_LOW));
}

TEST_CASE(test_hys_inf_trip_low_prev_range)
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

        /* Inf sample with TRIP_LOW prev: policy applies */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval_hys(&plan, INFINITY, 5.0f,
                                                  THRESHOLD_SEV_TRIP_LOW));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval_hys(&plan, -INFINITY, 5.0f,
                                                  THRESHOLD_SEV_TRIP_LOW));
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout,
                "\n=== Running test_nonfinite_samples unit tests ===\n\n");

        /* NaN samples: DISCRETE_WARN */
        run_test(test_nan_discrete_warn_failsafe_trip,
                 "test_nan_discrete_warn_failsafe_trip");
        run_test(test_nan_discrete_warn_deescalate_warn,
                 "test_nan_discrete_warn_deescalate_warn");
        run_test(test_nan_discrete_warn_ignore_invalid,
                 "test_nan_discrete_warn_ignore_invalid");
        run_test(test_nan_discrete_warn_default_returns_invalid,
                 "test_nan_discrete_warn_default_returns_invalid");

        /* NaN samples: DISCRETE_TRIP */
        run_test(test_nan_discrete_trip_failsafe_trip,
                 "test_nan_discrete_trip_failsafe_trip");
        run_test(test_nan_discrete_trip_deescalate_warn,
                 "test_nan_discrete_trip_deescalate_warn");
        run_test(test_nan_discrete_trip_ignore_invalid,
                 "test_nan_discrete_trip_ignore_invalid");
        run_test(test_nan_discrete_trip_default_returns_invalid,
                 "test_nan_discrete_trip_default_returns_invalid");

        /* NaN samples: LOWER type */
        run_test(test_nan_lower_failsafe_trip, "test_nan_lower_failsafe_trip");
        run_test(test_nan_lower_deescalate_warn,
                 "test_nan_lower_deescalate_warn");
        run_test(test_nan_lower_ignore_invalid,
                 "test_nan_lower_ignore_invalid");
        run_test(test_nan_lower_default_returns_invalid,
                 "test_nan_lower_default_returns_invalid");

        /* Inf samples: DISCRETE_WARN */
        run_test(test_inf_discrete_warn_failsafe_trip,
                 "test_inf_discrete_warn_failsafe_trip");
        run_test(test_inf_discrete_warn_deescalate_warn,
                 "test_inf_discrete_warn_deescalate_warn");
        run_test(test_inf_discrete_warn_ignore_invalid,
                 "test_inf_discrete_warn_ignore_invalid");

        /* Inf samples: DISCRETE_TRIP */
        run_test(test_inf_discrete_trip_failsafe_trip,
                 "test_inf_discrete_trip_failsafe_trip");
        run_test(test_inf_discrete_trip_deescalate_warn,
                 "test_inf_discrete_trip_deescalate_warn");
        run_test(test_inf_discrete_trip_ignore_invalid,
                 "test_inf_discrete_trip_ignore_invalid");

        /* Inf samples: LOWER type */
        run_test(test_inf_lower_failsafe_trip, "test_inf_lower_failsafe_trip");
        run_test(test_inf_lower_deescalate_warn,
                 "test_inf_lower_deescalate_warn");
        run_test(test_inf_lower_ignore_invalid,
                 "test_inf_lower_ignore_invalid");

        /* Hysteresis NaN with TRIP_LOW prev */
        run_test(test_hys_nan_trip_low_prev_range,
                 "test_hys_nan_trip_low_prev_range");
        run_test(test_hys_nan_trip_low_prev_lower,
                 "test_hys_nan_trip_low_prev_lower");
        run_test(test_hys_nan_trip_low_prev_deescalate,
                 "test_hys_nan_trip_low_prev_deescalate");
        run_test(test_hys_nan_trip_low_prev_ignore,
                 "test_hys_nan_trip_low_prev_ignore");
        run_test(test_hys_inf_trip_low_prev_range,
                 "test_hys_inf_trip_low_prev_range");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
