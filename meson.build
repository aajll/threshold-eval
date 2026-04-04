project(
  'threshold_eval',
  'c',
  version: '1.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'Configurable threshold evaluation library for classifying sampled values against validated warning/trip limits with deterministic, policy-driven behavior.'

ver = meson.project_version().split('.')
major = ver[0].to_int()
minor = ver[1].to_int()
patch = ver[2].to_int()

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('MAJOR', major)
conf.set('MINOR', minor)
conf.set('PATCH', patch)

generated_version_header = configure_file(
  input: 'config' / 'threshold_eval_version.h.in',
  output: 'threshold_eval_version.h',
  configuration: conf,
  install: true,
  install_dir: get_option('includedir') / 'threshold_eval',
)

public_headers = include_directories('include')
generated_headers = include_directories('.')

library_sources = files('src/threshold_eval.c')

threshold_eval_lib = static_library(
  'threshold_eval',
  library_sources,
  include_directories: [public_headers, generated_headers],
  install: true,
)

install_headers(
  'include/threshold_eval.h',
  'include/threshold_eval_conf.h',
  subdir: 'threshold_eval',
)

threshold_eval_dep = declare_dependency(
  include_directories: [public_headers, generated_headers],
  link_with: threshold_eval_lib,
  sources: generated_version_header,
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
