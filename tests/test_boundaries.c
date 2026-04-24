/**
 * @file test_boundaries.c
 * @brief Unit tests for boundary conditions and epsilon dead-bands.
 */

#include "threshold_eval.h"
#include "test_harness.h"

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
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));

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
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = eps;
        cfg.policy = 0;
        threshold_plan_build(plan, &cfg);
}

/* --- HI dead-band (eps=1.0): effective trigger at 81.0 ------------------- */

TEST_CASE(test_boundary_hi_exact_at_threshold)
{
        /* hi=80, eps=1 -> dead-band [80, 81]; 80 > 81 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
}

TEST_CASE(test_boundary_hi_inside_deadband)
{
        /* 80.5 is inside dead-band (80, 81) -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));
}

TEST_CASE(test_boundary_hi_at_deadband_edge)
{
        /* sample == hi + eps = 81.0; 81 > 81 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 81.0f));
}

TEST_CASE(test_boundary_hi_past_deadband)
{
        /* 81.1 > 81 -> WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 81.1f));
}

/* --- HIHI dead-band (eps=1.0): effective trip trigger at 91.0 ------------ */

TEST_CASE(test_boundary_hihi_exact_at_threshold)
{
        /* hihi=90, eps=1 -> 90 > 91 is false; 90 > 81 is true -> WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
}

TEST_CASE(test_boundary_hihi_inside_deadband)
{
        /* 90.5 is inside dead-band (90, 91) -> WARN_HIGH, not TRIP */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.5f));
}

TEST_CASE(test_boundary_hihi_at_deadband_edge)
{
        /* sample == hihi + eps = 91.0; 91 > 91 is false -> WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 91.0f));
}

TEST_CASE(test_boundary_hihi_past_deadband)
{
        /* 91.1 > 91 -> TRIP_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 91.1f));
}

/* --- LO dead-band (eps=1.0): effective warn trigger at 19.0 -------------- */

TEST_CASE(test_boundary_lo_exact_at_threshold)
{
        /* lo=20, eps=1 -> dead-band [19, 20]; 20 < 19 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
}

TEST_CASE(test_boundary_lo_inside_deadband)
{
        /* 19.5 is inside dead-band (19, 20) -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.5f));
}

TEST_CASE(test_boundary_lo_at_deadband_edge)
{
        /* sample == lo - eps = 19.0; 19 < 19 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.0f));
}

TEST_CASE(test_boundary_lo_past_deadband)
{
        /* 18.9 < 19 -> WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 18.9f));
}

/* --- LOLO dead-band (eps=1.0): effective trip trigger at 9.0 ------------- */

TEST_CASE(test_boundary_lolo_exact_at_threshold)
{
        /* lolo=10, eps=1 -> 10 < 9 is false; 10 < 19 is true -> WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
}

TEST_CASE(test_boundary_lolo_inside_deadband)
{
        /* 9.5 is inside dead-band (9, 10) -> WARN_LOW, not TRIP */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.5f));
}

TEST_CASE(test_boundary_lolo_at_deadband_edge)
{
        /* sample == lolo - eps = 9.0; 9 < 9 is false -> WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.0f));
}

TEST_CASE(test_boundary_lolo_past_deadband)
{
        /* 8.9 < 9 -> TRIP_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 1.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, 8.9f));
}

/* --- Zero epsilon: no dead-band; strict inequality fires immediately ------ */

TEST_CASE(test_boundary_zero_eps_hi_exact)
{
        /* eps=0: 80 > 80 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
}

TEST_CASE(test_boundary_zero_eps_hi_tiny_above)
{
        /* eps=0: 80.01 > 80 -> WARN_HIGH with no dead-band */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 80.01f));
}

TEST_CASE(test_boundary_zero_eps_hihi_exact)
{
        /* eps=0: 90 > 90 is false; 90 > 80 is true -> WARN_HIGH */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
}

TEST_CASE(test_boundary_zero_eps_hihi_tiny_above)
{
        /* eps=0: 90.01 > 90 -> TRIP_HIGH immediately */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 90.01f));
}

TEST_CASE(test_boundary_zero_eps_lo_exact)
{
        /* eps=0: 20 < 20 is false -> OK */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
}

TEST_CASE(test_boundary_zero_eps_lo_tiny_below)
{
        /* eps=0: 19.99 < 20 -> WARN_LOW with no dead-band */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 19.99f));
}

TEST_CASE(test_boundary_zero_eps_lolo_exact)
{
        /* eps=0: 10 < 10 is false; 10 < 20 is true -> WARN_LOW */
        threshold_plan_t plan;
        build_range_eps(&plan, 0.0f);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
}

TEST_CASE(test_boundary_zero_eps_lolo_tiny_below)
{
        /* eps=0: 9.99 < 10 -> TRIP_LOW immediately */
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
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, 80.0f + THRESHOLD_EVAL_EPSILON * 0.5f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, 80.0f + THRESHOLD_EVAL_EPSILON));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_WARN_HIGH,
            threshold_plan_eval(&plan, 80.0f + THRESHOLD_EVAL_EPSILON * 2.0f));
}

TEST_CASE(test_boundary_default_eps_lo_boundary)
{
        /* lo-eps = 20.0 - 0.001 = 19.999.  Values at 20, at 20-eps/2, and at
         * the edge 20-eps stay OK; a value at 20-eps*2 must trigger WARN_LOW.
         */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, 20.0f - THRESHOLD_EVAL_EPSILON * 0.5f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, 20.0f - THRESHOLD_EVAL_EPSILON));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_WARN_LOW,
            threshold_plan_eval(&plan, 20.0f - THRESHOLD_EVAL_EPSILON * 2.0f));
}

/* --- Per-type boundary tests: LOWER type --------------------------------- */

TEST_CASE(test_boundary_lower_type_all_boundaries)
{
        /* Verify epsilon dead-band on both LOLO and LO for LOWER type. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* LO boundary: dead-band [19, 20] -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 20.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 19.0f));
        /* Just past LO dead-band -> WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 18.9f));
        /* LOLO boundary: dead-band [9, 10] -> WARN_LOW */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 10.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW,
                          threshold_plan_eval(&plan, 9.0f));
        /* Just past LOLO dead-band -> TRIP_LOW */
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
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* HI boundary: dead-band [80, 81] -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 80.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 81.0f));
        /* Just past HI dead-band -> WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 81.1f));
        /* HIHI boundary: dead-band [90, 91] -> WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 90.5f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 91.0f));
        /* Just past HIHI dead-band -> TRIP_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, 91.1f));
}

/* --- Per-type boundary tests: DISCRETE_WARN ------------------------------ */

TEST_CASE(test_boundary_discrete_warn_eps)
{
        /* DISCRETE_WARN: hi=50, eps=2 -> dead-band [50, 52]. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_WARN;
        cfg.hi = 50.0f;
        cfg.epsilon = 2.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* At threshold -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 50.0f));
        /* Inside dead-band -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 51.0f));
        /* At dead-band edge hi+eps=52; 52 > 52 is false -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 52.0f));
        /* Past dead-band -> WARN_HIGH */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH,
                          threshold_plan_eval(&plan, 52.1f));
}

/* --- Per-type boundary tests: DISCRETE_TRIP ------------------------------ */

TEST_CASE(test_boundary_discrete_trip_eps)
{
        /* DISCRETE_TRIP: hihi=100, eps=2 -> dead-band [100, 102]. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_DISCRETE_TRIP;
        cfg.hihi = 100.0f;
        cfg.epsilon = 2.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* At threshold -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 100.0f));
        /* Inside dead-band -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 101.0f));
        /* At dead-band edge hihi+eps=102; 102 > 102 is false -> OK */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 102.0f));
        /* Past dead-band -> TRIP_HIGH */
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
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = -90.0f;
        cfg.lo = -80.0f;
        cfg.hi = -20.0f;
        cfg.hihi = -10.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Middle -> OK */
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
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.hi = 80.0f;
        cfg.hihi = 95.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* First crossing: 82 > hi+eps=81 -> WARN_HIGH */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 82.0f, 3.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s1);

        /* Drops to 79: without hys would clear (79 < 81), but with hys the
         * effective trigger is 78, so 79 > 78 -> stays WARN_HIGH */
        threshold_severity_t s2 = threshold_plan_eval_hys(
            &plan, 79.0f, 3.0f, THRESHOLD_SEV_WARN_HIGH);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_HIGH, s2);

        /* Drops to 78: at the dead-band edge; 78 > 78 is false -> clears OK */
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
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.lolo = 5.0f;
        cfg.lo = 20.0f;
        cfg.epsilon = 1.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* First crossing: 18 < lo-eps=19 -> WARN_LOW */
        threshold_severity_t s1 =
            threshold_plan_eval_hys(&plan, 18.0f, 3.0f, THRESHOLD_SEV_OK);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s1);

        /* Rises to 21: without hys would clear (21 >= 19), but with hys the
         * effective trigger is 22, so 21 < 22 -> stays WARN_LOW */
        threshold_severity_t s2 =
            threshold_plan_eval_hys(&plan, 21.0f, 3.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_WARN_LOW, s2);

        /* Rises to 22: at the dead-band edge; 22 < 22 is false -> clears OK */
        threshold_severity_t s3 =
            threshold_plan_eval_hys(&plan, 22.0f, 3.0f, THRESHOLD_SEV_WARN_LOW);
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, s3);
}

/* ================ Zero threshold edge case tests ============================
 */

TEST_CASE(test_boundary_zero_thresholds)
{
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = -10.0f;
        cfg.lo = -5.0f;
        cfg.hi = 5.0f;
        cfg.hihi = 10.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;

        threshold_status_t status = threshold_plan_build(&plan, &cfg);
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK, status);

        /* Values around zero */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 0.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, -0.0f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK, threshold_plan_eval(&plan, 0.001f));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_OK,
                          threshold_plan_eval(&plan, -0.001f));
}

/* ================ INT_MIN / INT_MAX boundary tests ======================== */

TEST_CASE(test_boundary_int_min_max_conversion)
{
        /* Test that values near -THRESHOLD_EVAL_MAX and +THRESHOLD_EVAL_MAX
         * don't cause overflow/underflow or incorrect severity classification.
         * The library casts between float and int internally for validation. */
        threshold_config_t cfg;
        threshold_plan_t plan;
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_RANGE;
        cfg.lolo = -THRESHOLD_EVAL_MAX + 1.0f;
        cfg.lo = -THRESHOLD_EVAL_MAX + 100.0f;
        cfg.hi = THRESHOLD_EVAL_MAX - 100.0f;
        cfg.hihi = THRESHOLD_EVAL_MAX - 1.0f;
        cfg.epsilon = 0.0f;
        cfg.policy = 0;
        threshold_plan_build(&plan, &cfg);

        /* Test values at exact limits */
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_LOW,
                          threshold_plan_eval(&plan, -THRESHOLD_EVAL_MAX));
        TEST_ASSERT_EQUAL(THRESHOLD_SEV_TRIP_HIGH,
                          threshold_plan_eval(&plan, THRESHOLD_EVAL_MAX));

        /* Test values just inside range */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, -THRESHOLD_EVAL_MAX + 101.0f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_OK,
            threshold_plan_eval(&plan, THRESHOLD_EVAL_MAX - 101.0f));

        /* Test values just outside range (at thresholds) */
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_TRIP_LOW,
            threshold_plan_eval(&plan, -THRESHOLD_EVAL_MAX + 0.5f));
        TEST_ASSERT_EQUAL(
            THRESHOLD_SEV_TRIP_HIGH,
            threshold_plan_eval(&plan, THRESHOLD_EVAL_MAX - 0.5f));
}

/* ================ Repeated evaluation consistency tests =================== */

TEST_CASE(test_eval_repeated_consistency)
{
        /* Verify that repeated evaluations of the same value with the same
         * plan produce identical results. This tests that the library is
         * truly stateless and deterministic. */
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
        threshold_plan_build(&plan, &cfg);

        /* Test values across all severity ranges */
        float test_values[] = {5.0f, 15.0f, 50.0f, 85.0f, 95.0f};
        size_t num_values = sizeof(test_values) / sizeof(test_values[0]);

        for (size_t i = 0; i < num_values; i++) {
                threshold_severity_t first_result =
                    threshold_plan_eval(&plan, test_values[i]);

                /* Evaluate 1000 times and assert all results match */
                for (int j = 0; j < 1000; j++) {
                        threshold_severity_t result =
                            threshold_plan_eval(&plan, test_values[i]);
                        TEST_ASSERT_EQUAL(first_result, result);
                }
        }
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_boundaries unit tests ===\n\n");

        run_test(test_eval_at_boundary_with_epsilon,
                 "test_eval_at_boundary_with_epsilon");

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
        run_test(test_boundary_lo_past_deadband,
                 "test_boundary_lo_past_deadband");
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
        run_test(test_boundary_zero_thresholds,
                 "test_boundary_zero_thresholds");
        run_test(test_boundary_int_min_max_conversion,
                 "test_boundary_int_min_max_conversion");
        run_test(test_eval_repeated_consistency,
                 "test_eval_repeated_consistency");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
