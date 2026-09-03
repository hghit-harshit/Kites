# Running Tests

Kites uses [GoogleTest](https://github.com/google/googletest) for unit testing, integrated with CMake/CTest. Two test targets are available:

- **`kites_test`** — plain test build, no instrumentation. Use this for everyday local testing.
- **`kites_test_cov`** — same test suite, built with coverage instrumentation (`--coverage`), plus a `coverage` target that generates an HTML report via [`gcovr`](https://gcovr.com/).

Both targets are produced from a single configure step; you choose which one to build/run afterward.

## Prerequisites

- CMake 3.24+
- Ninja
- A GCC- or Clang-style compiler (coverage instrumentation requires this — see [Notes](#notes))
- [`gcovr`](https://gcovr.com/) on your `PATH` (only required if configuring with `ENABLE_COVERAGE=ON`, which is the default)

## Configure

Both test targets live under the same configure preset:

```sh
cmake --preset test
```

## Running tests (no coverage)

```sh
cmake --build --preset test-only
ctest --preset test-only
```

This builds and runs `kites_test` only — the fast path for everyday local test runs.

## Running tests (with coverage)

```sh
cmake --build --preset test-coverage
ctest --preset test-coverage
```

This builds and runs `kites_test_cov`, whose CTest test names are prefixed with `cov_` so they don't collide with the plain suite's test names.

### Generating the HTML coverage report

The `coverage` target runs the instrumented test suite via CTest and then invokes `gcovr` to produce an HTML report:

```sh
cmake --build build/test --target coverage
```

The report is written to `build/test/coverage/index.html`.

## Build options

Set via `-D<OPTION>=ON|OFF` at configure time, or in `CMakePresets.json`:

| Option              | Default | Description                                      |
|---------------------|---------|---------------------------------------------------|
| `DISABLE_GUI`        | `ON`    | Disable GUI components during tests               |
| `VM_DEBUG_PRINTS`     | `OFF`   | Enable debug prints in the VM during tests        |
| `ENABLE_COVERAGE`     | `ON`    | Build the `kites_test_cov` target and `coverage` target |

## Notes

- Coverage instrumentation (`--coverage`) requires GCC or Clang; configuring with `ENABLE_COVERAGE=ON` on another compiler will fail with a clear error.
- If you're on Windows with a Qt-bundled MinGW toolchain, confirm it actually ships `libgcov.a` before relying on coverage builds — some trimmed-down distributions omit it. You can check with:
  ```sh
  g++ -print-file-name=libgcov.a
  ```
  A real path back means it's present; the bare filename echoed back means it isn't found.