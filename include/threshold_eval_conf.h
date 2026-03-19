/**
 * @file
 *    threshold_eval_conf.h
 *
 * @brief
 *    Public configuration header for threshold-eval.
 *
 * @details
 *    This header provides configuration options for the threshold_eval module.
 *    It is included automatically by threshold_eval.h, but applications may
 *    also include it directly before threshold_eval.h to keep configuration in
 *    one place. Default values are provided if not overridden.
 *
 * @note
 *    Override any configuration option by defining it before including
 *    this file. Example:
 *    @code
 *    #define THRESHOLD_EVAL_MAX 1000000.0f
 *    #include "threshold_eval_conf.h"
 *    #include "threshold_eval.h"
 *    @endcode
 *
 */
#ifndef THRESHOLD_EVAL_CONF_H_
#define THRESHOLD_EVAL_CONF_H_

/* ================ CONFIGURATION =========================================== */

/* ---------------- Threshold Range ----------------------------------------- */

#ifndef THRESHOLD_EVAL_MAX
/**
 * @def THRESHOLD_EVAL_MAX
 * @brief Maximum absolute threshold value.
 *
 * @details
 *    Thresholds outside [-THRESHOLD_EVAL_MAX, +THRESHOLD_EVAL_MAX] are
 *    rejected during plan validation. Set to a reasonable engineering maximum
 *    for your application domain.
 *
 * @note
 *    The default value of 4,000,000 covers most engineering measurement ranges
 *    (pressure, temperature, voltage, frequency). Override for applications
 *    with significantly different measurement scales.
 */
#define THRESHOLD_EVAL_MAX (4000000.0f)
#endif

/* ---------------- Comparison Epsilon -------------------------------------- */

#ifndef THRESHOLD_EVAL_EPSILON
/**
 * @def THRESHOLD_EVAL_EPSILON
 * @brief Default epsilon for threshold boundary comparisons.
 *
 * @details
 *    A small tolerance added/subtracted at threshold boundaries to prevent
 *    chatter from noise or quantization when a sampled value sits exactly at
 *    a threshold. Applied as: trip if sample < lolo - epsilon (lower) or
 *    sample > hihi + epsilon (upper).
 *
 * @note
 *    Set to 0.0f to disable boundary tolerance. Increase for high-noise
 *    signals. The value is also configurable per-plan via threshold_config_t.
 */
#define THRESHOLD_EVAL_EPSILON (1.0e-3f)
#endif

#endif /* THRESHOLD_EVAL_CONF_H_ */
