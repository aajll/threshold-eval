# Integration guide

This guide shows how to add `threshold-eval` to a project, configure it, build it, and run its tests. The API is described in [api.md](api.md), and the plan model and its guarantees in [design.md](design.md).

## Requirements

- A C11 compiler with IEEE 754 single-precision `float`.
- A C library that provides `<math.h>` with the `isfinite` and `NAN` macros. The library calls no `<math.h>` function, so it needs no maths library at link time.
- For the tests: Meson, Ninja, and Python 3.

## Copy-in

Copy three files into your source tree:

```
include/threshold_eval.h
include/threshold_eval_conf.h
src/threshold_eval.c
```

Keep both headers in the same directory, because `threshold_eval.h` includes `threshold_eval_conf.h`. Then include the public header:

```c
#include "threshold_eval.h"
```

`threshold_eval.h` defines the version macros described in [api.md](api.md#version).

## Meson subproject

Put the repository in `subprojects/threshold_eval`, as a checkout or through a wrap file, then declare the dependency:

```meson
threshold_eval_dep = dependency('threshold_eval', fallback: ['threshold_eval', 'threshold_eval_dep'])
```

The build calls `meson.override_dependency('threshold_eval', ...)`, so other subprojects in the same build find `threshold_eval` by name. `threshold_eval_dep` links the static library and adds the include directories, including the one that holds the generated version header. Tests are off unless the `build_tests` option is set.

| Option | Default | Effect |
| --- | --- | --- |
| `build_tests` | `false` | Builds the unit tests and the documentation example test. |

## Installed package

```sh
meson setup build --buildtype=release -Dbuild_tests=false --prefix=/usr/local
meson compile -C build
meson install -C build
cc app.c $(pkg-config --cflags --libs threshold_eval) -o app
```

The install contains the static archive `libthreshold_eval.a`, the headers in a `threshold_eval/` subdirectory, and `threshold_eval.pc`. The `pkg-config` flags add that directory to the include path, so the same include works as in a copy-in build. Without `pkg-config`, include the namespaced path instead:

<!-- snippet: skip -->
```c
#include <threshold_eval/threshold_eval.h>
```

## Configuration

`threshold_eval_conf.h` holds two options, each behind an `#ifndef` guard, so a `-D` definition replaces the shipped default. They are described in [api.md](api.md#configuration-macros).

Both are read only by `src/threshold_eval.c`: `THRESHOLD_EVAL_MAX` in the range check of `threshold_plan_build()`, and `THRESHOLD_EVAL_EPSILON` in `threshold_config_init()`. A definition placed in a caller's source before `#include "threshold_eval.h"` therefore changes nothing in the library. `threshold_eval.h` includes `threshold_eval_conf.h` with quotes, so the compiler finds the copy beside it before any directory on the include path, and a project cannot substitute its own `threshold_eval_conf.h` either. Define them with `-D` for the library build. To change the epsilon of one plan, set `epsilon` in its configuration instead.

In a Meson build, set them in the top-level project's `c_args`, which also apply to subprojects:

```sh
meson setup build "-Dc_args=-DTHRESHOLD_EVAL_MAX=1.0e5f"
```

A per-target `c_args` or an `add_project_arguments` call in the parent project does not reach the subproject, and an installed library keeps the values it was built with.

## Build and test

```sh
# Library only
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# All tests under ASan and UBSan
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose
```

Coverage, formatting and MISRA commands are in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Cross builds

The consuming firmware owns its cross file, linker command file, and memory placement. No cross file ships with this project.

CI compiles the library with warnings as errors using two releases of a vendor C11 compiler for a 16-bit-MAU DSP target with a single-precision FPU, under both of its object formats. This covers the source dialect and type assumptions only; linking and execution remain checks for the consumer.

On such a target `CHAR_BIT` is 16 and the enumerations are 16 bits wide, so `sizeof` of the configuration and plan structures differs from a host build. Nothing in the API depends on their size or layout.
