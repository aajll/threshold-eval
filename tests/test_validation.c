/**
 * SPDX-License-Identifier: MIT
 *
 * @file test_validation.c
 * @brief Unit tests for threshold_plan_build validation.
 */

#include "test_harness.h"
#include "threshold_eval.h"

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

/* ================ Per-field validation matrix ============================= */

/* Field indices into the per-type tables below. */
enum {
        FIELD_LOLO,
        FIELD_LO,
        FIELD_HI,
        FIELD_HIHI,
        FIELD_COUNT
};

/* A valid, ordered value for each field. */
static const float field_value[FIELD_COUNT] = {10.0f, 20.0f, 80.0f, 90.0f};

struct type_fields {
        threshold_type_t type;
        bool required[FIELD_COUNT];
};

static const struct type_fields type_fields[] = {
    {THRESHOLD_TYPE_RANGE, {true, true, true, true}},
    {THRESHOLD_TYPE_UPPER, {false, false, true, true}},
    {THRESHOLD_TYPE_LOWER, {true, true, false, false}},
    {THRESHOLD_TYPE_DISCRETE_WARN, {false, false, true, false}},
    {THRESHOLD_TYPE_DISCRETE_TRIP, {false, false, false, true}},
};

static float *
config_field(threshold_config_t *cfg, size_t field)
{
        float *const fields[FIELD_COUNT] = {&cfg->lolo, &cfg->lo, &cfg->hi,
                                            &cfg->hihi};
        return fields[field];
}

/* Valid strict config for a type: required fields set, the rest NAN. */
static void
make_valid_config(threshold_config_t *cfg, const struct type_fields *tf)
{
        threshold_config_init(cfg);
        cfg->type = tf->type;
        cfg->policy = THRESHOLD_POLICY_STRICT_CONFIG;
        for (size_t f = 0u; f < FIELD_COUNT; f++) {
                if (tf->required[f]) {
                        *config_field(cfg, f) = field_value[f];
                }
        }
}

static threshold_status_t
build_status(const threshold_config_t *cfg)
{
        threshold_plan_t plan;
        const threshold_status_t status = threshold_plan_build(&plan, cfg);
        TEST_ASSERT_EQUAL(status == THRESHOLD_STATUS_OK, plan.valid);
        return status;
}

TEST_CASE(test_plan_build_each_required_field_missing_fails)
{
        const float missing[] = {NAN, INFINITY, -INFINITY};

        for (size_t t = 0u; t < sizeof(type_fields) / sizeof(type_fields[0]);
             t++) {
                for (size_t f = 0u; f < FIELD_COUNT; f++) {
                        if (!type_fields[t].required[f]) {
                                continue;
                        }
                        for (size_t m = 0u;
                             m < sizeof(missing) / sizeof(missing[0]); m++) {
                                threshold_config_t cfg;
                                make_valid_config(&cfg, &type_fields[t]);
                                TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                                                  build_status(&cfg));
                                *config_field(&cfg, f) = missing[m];
                                TEST_ASSERT_EQUAL(THRESHOLD_STATUS_MISSING,
                                                  build_status(&cfg));
                        }
                }
        }
}

TEST_CASE(test_plan_build_each_required_field_out_of_range_fails)
{
        const float too_large[] = {2.0f * THRESHOLD_EVAL_MAX,
                                   -2.0f * THRESHOLD_EVAL_MAX};

        for (size_t t = 0u; t < sizeof(type_fields) / sizeof(type_fields[0]);
             t++) {
                for (size_t f = 0u; f < FIELD_COUNT; f++) {
                        if (!type_fields[t].required[f]) {
                                continue;
                        }
                        for (size_t v = 0u;
                             v < sizeof(too_large) / sizeof(too_large[0]);
                             v++) {
                                threshold_config_t cfg;
                                make_valid_config(&cfg, &type_fields[t]);
                                /* Out of range is checked before order. */
                                cfg.policy = 0u;
                                *config_field(&cfg, f) = too_large[v];
                                TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OUT_OF_RANGE,
                                                  build_status(&cfg));
                        }
                }
        }
}

TEST_CASE(test_plan_build_each_unused_field_is_extra_when_strict)
{
        for (size_t t = 0u; t < sizeof(type_fields) / sizeof(type_fields[0]);
             t++) {
                for (size_t f = 0u; f < FIELD_COUNT; f++) {
                        if (type_fields[t].required[f]) {
                                continue;
                        }
                        threshold_config_t cfg;
                        make_valid_config(&cfg, &type_fields[t]);
                        *config_field(&cfg, f) = field_value[f];
                        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_EXTRA,
                                          build_status(&cfg));

                        /* Without STRICT_CONFIG the extra field is ignored. */
                        cfg.policy = 0u;
                        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                                          build_status(&cfg));
                }
        }
}

TEST_CASE(test_plan_build_upper_lower_reorder)
{
        threshold_config_t cfg;
        threshold_plan_t plan;

        /* LOWER out of order is swapped. */
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_LOWER;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;
        cfg.lolo = 20.0f;
        cfg.lo = 10.0f;
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                          threshold_plan_build(&plan, &cfg));
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);

        /* LOWER in order is kept. */
        cfg.lolo = 10.0f;
        cfg.lo = 20.0f;
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                          threshold_plan_build(&plan, &cfg));
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 10.0f, plan.lolo);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 20.0f, plan.lo);

        /* UPPER out of order is swapped. */
        threshold_config_init(&cfg);
        cfg.type = THRESHOLD_TYPE_UPPER;
        cfg.policy = THRESHOLD_POLICY_ALLOW_REORDER;
        cfg.hi = 90.0f;
        cfg.hihi = 80.0f;
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                          threshold_plan_build(&plan, &cfg));
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);

        /* UPPER in order is kept. */
        cfg.hi = 80.0f;
        cfg.hihi = 90.0f;
        TEST_ASSERT_EQUAL(THRESHOLD_STATUS_OK,
                          threshold_plan_build(&plan, &cfg));
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 80.0f, plan.hi);
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 90.0f, plan.hihi);
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

        run_test(test_plan_build_each_required_field_missing_fails,
                 "test_plan_build_each_required_field_missing_fails");
        run_test(test_plan_build_each_required_field_out_of_range_fails,
                 "test_plan_build_each_required_field_out_of_range_fails");
        run_test(test_plan_build_each_unused_field_is_extra_when_strict,
                 "test_plan_build_each_unused_field_is_extra_when_strict");
        run_test(test_plan_build_upper_lower_reorder,
                 "test_plan_build_upper_lower_reorder");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
