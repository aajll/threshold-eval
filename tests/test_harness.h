/**
 * @file test_harness.h
 * @brief Shared test harness for threshold-eval unit tests.
 *
 * @details
 *   This header provides the common test infrastructure used by all test
 *   modules in the tests/ directory. It includes assertion macros, string
 *   comparison, floating-point comparison, test case registration, and the
 *   test runner function.
 *
 *   Each test module should include this header instead of duplicating the
 *   harness code. No other headers should be needed beyond what is included
 *   here.
 *
 * @note
 *   This header is internal to the test suite and is not installed or
 *   distributed with the library.
 */

#ifndef TEST_HARNESS_H_
#define TEST_HARNESS_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================ Assertion macros ======================================== */

/** @brief Fail the test if the expression is false. */
#define TEST_ASSERT(expr)                                                      \
        do {                                                                   \
                if (!(expr)) {                                                 \
                        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__,         \
                                __LINE__, #expr);                              \
                        exit(EXIT_FAILURE);                                    \
                }                                                              \
        } while (0)

/** @brief Assert two values are equal. */
#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT((expected) == (actual))

/** @brief Assert a boolean expression is true. */
#define TEST_ASSERT_TRUE(expr)              TEST_ASSERT(!!(expr))

/** @brief Assert a boolean expression is false. */
#define TEST_ASSERT_FALSE(expr)             TEST_ASSERT(!(expr))

/** @brief Assert a pointer is not NULL. */
#define TEST_ASSERT_NOT_NULL(ptr)           TEST_ASSERT((ptr) != NULL)

/** @brief Assert two floats are within a delta. */
#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                      \
        TEST_ASSERT(fabsf((float)(actual) - (float)(expected))                 \
                    <= (float)(delta))

/** @brief Assert two strings are equal. */
#define TEST_ASSERT_EQUAL_STRING(expected, actual)                             \
        TEST_ASSERT(strcmp((expected), (actual)) == 0)

/* ================ Test case registration ================================== */

/** @brief Print a passing test name. */
#define TEST_PASS(name) fprintf(stdout, "PASS  %s\n", (name))

/** @brief Declare and define a test case function. */
#define TEST_CASE(name)                                                        \
        static void name(void);                                                \
        static void name(void)

/* ================ Test runner ============================================= */

/**
 * @brief Run a single test case with error reporting.
 * @param test_func  Function pointer to the test case.
 * @param name       Human-readable test name (for output).
 */
static void
run_test(void (*test_func)(void), const char *name)
{
        test_func();
        TEST_PASS(name);
}

#endif /* TEST_HARNESS_H_ */
