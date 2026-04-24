/**
 * @file test_helpers.c
 * @brief Unit tests for helper functions and main test runner.
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
                        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__,         \
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
            "DISCRETE_WARN", threshold_type_name(THRESHOLD_TYPE_DISCRETE_WARN));
        TEST_ASSERT_EQUAL_STRING(
            "DISCRETE_TRIP", threshold_type_name(THRESHOLD_TYPE_DISCRETE_TRIP));
        TEST_ASSERT_EQUAL_STRING("UNKNOWN",
                                 threshold_type_name((threshold_type_t)99));
}

TEST_CASE(test_status_str_returns_strings)
{
        TEST_ASSERT_EQUAL_STRING("OK",
                                 threshold_status_str(THRESHOLD_STATUS_OK));
        TEST_ASSERT_EQUAL_STRING(
            "Invalid argument",
            threshold_status_str(THRESHOLD_STATUS_INVALID_ARG));
        TEST_ASSERT_EQUAL_STRING(
            "Threshold exceeds maximum range",
            threshold_status_str(THRESHOLD_STATUS_OUT_OF_RANGE));
        TEST_ASSERT_EQUAL_STRING(
            "Required threshold is missing",
            threshold_status_str(THRESHOLD_STATUS_MISSING));
        TEST_ASSERT_EQUAL_STRING(
            "Thresholds are not in monotonic order",
            threshold_status_str(THRESHOLD_STATUS_ORDER));
        TEST_ASSERT_EQUAL_STRING(
            "Extra thresholds provided for plan type",
            threshold_status_str(THRESHOLD_STATUS_EXTRA));
        TEST_ASSERT_EQUAL_STRING(
            "Unknown status",
            threshold_status_str((threshold_status_t)-99));
}

TEST_CASE(test_all_status_codes_have_strings)
{
        /* Verify every defined status code maps to a non-empty string. */
        const threshold_status_t all_status[] = {
                THRESHOLD_STATUS_OK,
                THRESHOLD_STATUS_INVALID_ARG,
                THRESHOLD_STATUS_OUT_OF_RANGE,
                THRESHOLD_STATUS_MISSING,
                THRESHOLD_STATUS_ORDER,
                THRESHOLD_STATUS_EXTRA,
        };
        const size_t count = sizeof(all_status) / sizeof(all_status[0]);

        for (size_t i = 0; i < count; i++) {
                const char *s = threshold_status_str(all_status[i]);
                TEST_ASSERT_NOT_NULL(s);
                TEST_ASSERT(s[0] != '\0');
        }
}

/* ================ Main ==================================================== */

int
main(void)
{
        fprintf(stdout, "\n=== Running test_helpers unit tests ===\n\n");

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
        run_test(test_all_status_codes_have_strings,
                 "test_all_status_codes_have_strings");

        fprintf(stdout, "\n=== All tests passed ===\n\n");
        return EXIT_SUCCESS;
}
