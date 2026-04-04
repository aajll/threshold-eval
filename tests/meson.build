# Test executables for threshold-eval

test_config = executable(
  'test_config',
  files('test_config.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_validation = executable(
  'test_validation',
  files('test_validation.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_evaluation = executable(
  'test_evaluation',
  files('test_evaluation.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_hysteresis = executable(
  'test_hysteresis',
  files('test_hysteresis.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_policies = executable(
  'test_policies',
  files('test_policies.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_boundaries = executable(
  'test_boundaries',
  files('test_boundaries.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_helpers = executable(
  'test_helpers',
  files('test_helpers.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test_threshold_ordering = executable(
  'test_threshold_ordering',
  files('test_threshold_ordering.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

# Individual test cases
test('threshold_eval_config', test_config)
test('threshold_eval_validation', test_validation)
test('threshold_eval_evaluation', test_evaluation)
test('threshold_eval_hysteresis', test_hysteresis)
test('threshold_eval_policies', test_policies)
test('threshold_eval_boundaries', test_boundaries)
test('threshold_eval_helpers', test_helpers)
test('threshold_eval_ordering', test_threshold_ordering)

test_edge_cases = executable(
  'test_edge_cases',
  files('test_edge_cases.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test('threshold_eval_edge_cases', test_edge_cases)
