test_exe = executable(
  'test_threshold_eval',
  files('test_threshold_eval.c'),
  dependencies: [threshold_eval_dep],
  include_directories: [public_headers, generated_headers],
)

test('threshold_eval module', test_exe)
