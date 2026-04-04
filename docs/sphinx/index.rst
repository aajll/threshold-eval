threshold-eval Documentation
============================

Configurable threshold evaluation library for classifying sampled values
against validated warning/trip limits with deterministic, policy-driven
behavior.

Table of Contents
-----------------

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   api/modules

API Reference
-------------

The threshold-eval library provides a compact C API for evaluating sampled
values against configurable threshold bands with policy-driven handling of
invalid inputs, hysteresis support, and epsilon dead-bands.

The full symbol-level reference is available in the
`API Reference <api/modules.html>`_.

Quick Start
-----------

.. code-block:: c

   #include "threshold_eval.h"

   int main(void)
   {
           threshold_config_t cfg;
           threshold_plan_t plan;

           /* Initialise config with safe defaults */
           threshold_config_init(&cfg);

           /* Configure a full-range voltage monitor (V) */
           cfg.type   = THRESHOLD_TYPE_RANGE;
           cfg.lolo   = 10.5f;   /* under-voltage trip    */
           cfg.lo     = 11.0f;   /* under-voltage warning */
           cfg.hi     = 14.5f;   /* over-voltage warning  */
           cfg.hihi   = 15.0f;   /* over-voltage trip     */
           cfg.policy = THRESHOLD_POLICY_FAILSAFE;

           /* Build and validate the plan once at startup */
           if (threshold_plan_build(&plan, &cfg) != THRESHOLD_STATUS_OK) {
                   return 1;
           }

           /* Evaluate a sample at runtime (pure function, no side effects) */
           float voltage = 12.8f;
           threshold_severity_t sev = threshold_plan_eval(&plan, voltage);

           if (threshold_severity_is_trip(sev)) {
                   /* trigger_protection(); */
           } else if (threshold_severity_is_warn(sev)) {
                   /* raise_alarm(); */
           }

           return 0;
   }

Building the Library
--------------------

threshold-eval uses Meson as its build system:

.. code-block:: bash

   # Library only (release)
   meson setup build --buildtype=release -Dbuild_tests=false
   meson compile -C build

   # With unit tests
   meson setup build --buildtype=debug -Dbuild_tests=true
   meson compile -C build
   meson test -C build --verbose
