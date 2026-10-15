# SPDX-License-Identifier: MIT

project(
  'threshold_eval',
  'c',
  version: '1.2.0',
  meson_version: '>=0.57.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'Configurable threshold evaluation library for classifying sampled values against validated warning/trip limits with deterministic, policy-driven behavior.'

# include/threshold_eval.h defines the version macros, so a copy-in build has them.
# Configuration fails when they differ from the project version.

fs = import('fs')
version_parts = meson.project_version().split('.')
version_macros = {
  'THRESHOLD_EVAL_VERSION_MAJOR': version_parts[0],
  'THRESHOLD_EVAL_VERSION_MINOR': version_parts[1],
  'THRESHOLD_EVAL_VERSION_PATCH': version_parts[2],
  'THRESHOLD_EVAL_VERSION': '"' + meson.project_version() + '"',
}
header_lines = []
foreach line : fs.read('include' / 'threshold_eval.h').split('\n')
  header_lines += ' '.join(line.split())
endforeach
foreach name, value : version_macros
  if '#define @0@ @1@'.format(name, value) not in header_lines
    error('include/threshold_eval.h must define @0@ as @1@'.format(name, value))
  endif
endforeach

public_headers = include_directories('include')

library_sources = files('src/threshold_eval.c')

threshold_eval_lib = static_library(
  'threshold_eval',
  library_sources,
  include_directories: public_headers,
  install: true,
)

install_headers(
  'include/threshold_eval.h',
  'include/threshold_eval_conf.h',
  subdir: 'threshold_eval',
)

threshold_eval_dep = declare_dependency(
  include_directories: public_headers,
  link_with: threshold_eval_lib,
)

meson.override_dependency('threshold_eval', threshold_eval_dep)

pkg = import('pkgconfig')
pkg.generate(
  threshold_eval_lib,
  description: project_description,
  filebase: 'threshold_eval',
  name: 'threshold-eval',
  subdirs: 'threshold_eval',
)

if get_option('build_tests')
  subdir('tests')
endif
