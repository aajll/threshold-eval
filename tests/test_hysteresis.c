/**
 * @file test_hysteresis.c
 * @brief Unit tests for threshold_plan_eval_hys.
 */

#include "threshold_eval.h"
#include "test_harness.h"

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
        threshold_severity_t sev2 =
            threshold_plan_eval_hys(&plan, 21.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, sev2);

        /* At 26: above adjusted threshold (25) -> clears to OK */
        threshold_severity_t sev3 =
            threshold_plan_eval_hys(&plan, 26.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
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

/* ================ Hysteresis edge case tests ================================
 */

TEST_CASE(test_eval_hys_inf_hysteresis_returns_invalid)
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

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        threshold_severity_t sev =
            threshold_plan_eval_hys(&plan, 50.0f, INFINITY, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_hys_nan_hysteresis_returns_invalid)
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

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        threshold_severity_t sev =
            threshold_plan_eval_hys(&plan, 50.0f, NAN, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_INVALID, sev);
}

TEST_CASE(test_eval_hys_multiple_consecutive_evaluations)
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
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* Start OK, cross into WARN_HIGH */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 81.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* Stay in WARN_HIGH (hysteresis prevents clearing) */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 82.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Cross into TRIP_HIGH */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 96.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s3);

        /* With hysteresis, HIHI lowered to 85 */
        threshold_severity_t s4 = threshold_plan_eval_hys(
            &plan, 91.0f, 5.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s4);

        /* Sample drops below adjusted hihi, now checks hi (80) */
        threshold_severity_t s5 = threshold_plan_eval_hys(
            &plan, 82.0f, 5.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s5);

        /* Clear to OK */
        threshold_severity_t s6 = threshold_plan_eval_hys(
            &plan, 74.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s6);
}

TEST_CASE(test_eval_hys_multiple_consecutive_low)
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
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* Start OK, cross into WARN_LOW */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 19.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s1);

        /* Stay in WARN_LOW (hysteresis prevents clearing) */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, 18.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s2);

        /* Cross into TRIP_LOW */
        threshold_severity_t s3 =
            threshold_plan_eval_hys(&plan, 5.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW, s3);

        /* With hysteresis, LOLO raised to 15 */
        threshold_severity_t s4 =
            threshold_plan_eval_hys(&plan, 12.0f, 5.0f, THRESHOLD_SEV_TRIP_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW, s4);

        /* Rise to OK (22 >= lolo-hys=20 and 22 >= lo=20) */
        threshold_severity_t s5 =
            threshold_plan_eval_hys(&plan, 22.0f, 5.0f, THRESHOLD_SEV_TRIP_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s5);

        /* Clear to OK */
        threshold_severity_t s6 =
            threshold_plan_eval_hys(&plan, 26.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s6);
}

TEST_CASE(test_eval_hys_none_type_returns_ok)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_NONE;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* NONE type always returns OK regardless of hysteresis or prev */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval_hys(&plan, 50.0f, 5.0f,
                                                  THRESHOLD_SEV_TRIP_HIGH));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval_hys(&plan, 50.0f, 5.0f,
                                                  THRESHOLD_SEV_TRIP_LOW));
}

TEST_CASE(test_eval_hys_discrete_warn_with_hysteresis)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* First crossing: 51 > hi=50 -> WARN_HIGH */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 51.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* With hysteresis, hi lowered to 45 */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 49.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Clear to OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 44.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

TEST_CASE(test_eval_hys_discrete_trip_with_hysteresis)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* First crossing: 101 > hihi=100 -> TRIP_HIGH */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 101.0f, 10.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s1);

        /* With hysteresis, hihi lowered to 90 */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 95.0f, 10.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s2);

        /* Clear to OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 89.0f, 10.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

TEST_CASE(test_eval_hys_upper_type_with_hysteresis)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* WARN_HIGH: hi lowered by hysteresis */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 85.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* hi lowered to 75, so 80 > 75 -> still WARN_HIGH */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 80.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Clear to OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 74.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

TEST_CASE(test_eval_hys_lower_type_with_hysteresis)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* WARN_LOW: lo raised by hysteresis */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 15.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s1);

        /* lo raised to 25, so 20 < 25 -> still WARN_LOW */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, 20.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s2);

        /* Clear to OK */
        threshold_severity_t s3 =
            threshold_plan_eval_hys(&plan, 26.0f, 5.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

TEST_CASE(test_eval_hys_invalid_policy_with_hysteresis)
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

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* Invalid sample (NaN) with FAILSAFE_TRIP policy returns TRIP */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s1);

        /* Next evaluation with NaN and hysteresis still applies policy */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_TRIP_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH, s2);
}

TEST_CASE(test_eval_hys_invalid_policy_deescalate_with_hysteresis)
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

        /* Invalid sample with DEESCALATE_WARN returns WARN */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* Next evaluation with NaN and hysteresis still applies policy */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);
}

TEST_CASE(test_eval_hys_invalid_policy_ignore_with_hysteresis)
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

        /* Invalid sample with IGNORE_INVALID returns OK */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s1);

        /* Next evaluation with NaN and hysteresis still applies policy */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, NAN, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s2);
}

TEST_CASE(test_eval_hys_prev_invalid_returns_ok)
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
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* When prev is INVALID, no hysteresis adjustment is applied */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 50.0f, 5.0f, THRESHOLD_SEV_INVALID);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s1);

        /* Same for OK - no adjustment */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, 50.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s2);
}

TEST_CASE(test_eval_hys_zero_epsilon_with_hysteresis)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* With zero epsilon and hysteresis, thresholds adjust exactly by
         * hysteresis */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 81.0f, 5.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* hi lowered to 75, so 79 > 75 -> still WARN_HIGH */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 79.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Clear to OK */
        threshold_severity_t s3 = threshold_plan_eval_hys(
            &plan, 74.0f, 5.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_hysteresis unit tests ===\n\n");

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
        run_test(test_eval_hys_inf_hysteresis_returns_invalid,
                 "test_eval_hys_inf_hysteresis_returns_invalid");
        run_test(test_eval_hys_nan_hysteresis_returns_invalid,
                 "test_eval_hys_nan_hysteresis_returns_invalid");
        run_test(test_eval_hys_multiple_consecutive_evaluations,
                 "test_eval_hys_multiple_consecutive_evaluations");
        run_test(test_eval_hys_multiple_consecutive_low,
                 "test_eval_hys_multiple_consecutive_low");
        run_test(test_eval_hys_none_type_returns_ok,
                 "test_eval_hys_none_type_returns_ok");
        run_test(test_eval_hys_discrete_warn_with_hysteresis,
                 "test_eval_hys_discrete_warn_with_hysteresis");
        run_test(test_eval_hys_discrete_trip_with_hysteresis,
                 "test_eval_hys_discrete_trip_with_hysteresis");
        run_test(test_eval_hys_upper_type_with_hysteresis,
                 "test_eval_hys_upper_type_with_hysteresis");
        run_test(test_eval_hys_lower_type_with_hysteresis,
                 "test_eval_hys_lower_type_with_hysteresis");
        run_test(test_eval_hys_invalid_policy_with_hysteresis,
                 "test_eval_hys_invalid_policy_with_hysteresis");
        run_test(test_eval_hys_invalid_policy_deescalate_with_hysteresis,
                 "test_eval_hys_invalid_policy_deescalate_with_hysteresis");
        run_test(test_eval_hys_invalid_policy_ignore_with_hysteresis,
                 "test_eval_hys_invalid_policy_ignore_with_hysteresis");
        run_test(test_eval_hys_prev_invalid_returns_ok,
                 "test_eval_hys_prev_invalid_returns_ok");
        run_test(test_eval_hys_zero_epsilon_with_hysteresis,
                 "test_eval_hys_zero_epsilon_with_hysteresis");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
