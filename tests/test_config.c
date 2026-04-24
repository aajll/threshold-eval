/**
 * @file test_config.c
 * @brief Unit tests for threshold_config_init.
 */

#include "threshold_eval.h"
#include "test_harness.h"

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

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_config unit tests ===\n\n");

        run_test(test_config_init_sets_defaults,
                 "test_config_init_sets_defaults");
        run_test(test_config_init_null_returns_error,
                 "test_config_init_null_returns_error");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
