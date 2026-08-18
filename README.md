# bignum-init-from-string

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-init-from-string/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-init-from-string/actions/workflows/ci.yml)
[![GitHub release](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-init-from-string?label=release)](https://github.com/kirill-bayborodov/bignum-init-from-string/releases/latest)

`bignum-init-from-string` is a standalone C/ASM module that initializes a `bignum_t` from a textual non-negative integer. The production path is an x86-64 YASM implementation conforming to the System V AMD64 ABI; a portable C implementation is retained as a reference and fallback. The parser supports bases 2 through 16 and automatic decimal/hexadecimal base selection.

The operation skips leading ASCII whitespace, accepts digits `0-9`, `a-f`, and `A-F`, optionally removes a `0x`/`0X` prefix for base 0 and base 16, and constructs the value using repeated `acc = acc * base + digit` operations. The implementation does not use `isspace`, `memset`, `memcpy`, `strtol`, `strtoull`, core initialization helpers, or other external calls from the production C routine. On errors, the destination state is unspecified and must not be used.

## Distribution

The module is intended to be used as a standalone component of the `bignum-lib` family. The required `bignum-core` component is included as a Git submodule at `libs/bignum-core`.

## Features

- **Dual implementation:** x86-64 YASM is the primary implementation and C11 is the reference implementation.
- **Typed core status API:** the function returns `bignum_init_from_string_status_t`, using the core success and null-argument statuses together with parser-specific `BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE`, `BIGNUM_INIT_FROM_STRING_ERROR_EMPTY`, and `BIGNUM_INIT_FROM_STRING_ERROR_PARSE` values.
- **Base support:** explicit bases from 2 through 16 are supported; base 0 selects hexadecimal for `0x`/`0X` and decimal otherwise.
- **Digit handling:** decimal and hexadecimal letters are accepted case-insensitively; fractional notation, signs, and unsupported digits are rejected.
- **Overflow detection:** values that require more than `BIGNUM_CAPACITY` 64-bit words return `BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW`.
- **Deterministic verification:** tests cover decimal, hexadecimal, prefixes, automatic base selection, whitespace, empty strings, invalid bases, invalid digits, maximum capacity, and overflow.
- **Extended verification:** canaries, invalid-input checks, adjacent-object safety, and 100,000 deterministic fuzz/reference cases are covered.
- **Thread-safety testing:** independent destination objects are parsed concurrently.
- **Reproducible benchmarks:** ST and MT benchmarks support deterministic seeds, fingerprints, checksums, warm-up calls, input lengths, and `all_zero`, `all_nonzero`, and `mixed` modes.
- **Perf workflow:** the unchanged template Makefile provides `perf record`, repeated `perf stat`, raw `perf.data` retention, runtime validation, and comparison targets.

## Dependencies

| Dependency | Purpose |
|---|---|
| `make` | Build, test, lint, benchmark, and distribution targets |
| `gcc` | C compilation and linking |
| `yasm` | x86-64 assembly compilation |
| `cppcheck` | Static analysis |
| `perf` | Performance counters and sampling profiles |
| `taskset` | CPU affinity control |
| `pthread` | Multithreaded tests and benchmarks |

Clone the repository with its submodule:

```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-init-from-string.git
cd bignum-init-from-string
```

For an existing clone, initialize the submodule with:

```bash
git submodule update --init --recursive
```

## API

The public API is declared in `include/bignum_init_from_string.h`:

```c
bignum_init_from_string_status_t bignum_init_from_string(
    bignum_t *dst,
    const char *str,
    int base);
```

The parser-specific status values are exposed by the public header for compatibility with the core `bignum_init_from_string_status_t` type:

```c
#define BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE (-3)
#define BIGNUM_INIT_FROM_STRING_ERROR_EMPTY    (-4)
#define BIGNUM_INIT_FROM_STRING_ERROR_PARSE    (-5)
```

### Contract

| Condition | Return value | Destination behavior |
|---|---|---|
| `dst == NULL` | `BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG` | No destination access |
| `str == NULL` | `BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG` | No destination access |
| `base < 2`, `base == 1`, or `base > 16` except `base == 0` | `BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE` | The destination state is unspecified |
| Empty string after leading whitespace | `BIGNUM_INIT_FROM_STRING_ERROR_EMPTY` | The destination state is unspecified |
| Only `0x`/`0X` remains after prefix removal | `BIGNUM_INIT_FROM_STRING_ERROR_EMPTY` | The destination state is unspecified |
| Invalid digit or digit not representable in the selected base | `BIGNUM_INIT_FROM_STRING_ERROR_PARSE` | The destination state is unspecified |
| Value exceeds `BIGNUM_CAPACITY` words | `BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW` | The destination state is unspecified |
| Valid input that fits | `BIGNUM_INIT_FROM_STRING_SUCCESS` | Canonical `bignum_t` value is written |

Base 0 selects base 16 only when the first non-whitespace characters are `0x` or `0X`; otherwise it selects base 10. Base 16 accepts the same prefix explicitly. Leading ASCII whitespace consists of space, tab, newline, vertical tab, form feed, and carriage return. Signs and decimal points are not supported.

A successful call stores the value in little-endian word order. Zero has `len == 0`, while every nonzero result has a nonzero highest word. All words beyond `len` are zero because the destination is cleared before parsing.

Example:

```c
#include "bignum_init_from_string.h"

int initialize_value(bignum_t *dst)
{
    bignum_init_from_string_status_t status =
        bignum_init_from_string(dst, "  0xDEADBEEF", 0);

    return status == BIGNUM_INIT_FROM_STRING_SUCCESS ? 0 : -1;
}
```

After this call, `dst->len == 1` and `dst->words[0] == 0xDEADBEEF`. The caller must not inspect `dst` after an error; it must reinitialize the object before using it again.

## Build and test

Build the release object and submodule:

```bash
make build CONFIG=release
```

The production object is generated at:

```text
build/bignum_init_from_string.o
```

Run the full deterministic, extended, multithreaded, and integration-runner suite against the ASM implementation:

```bash
make test CONFIG=release
```

The expected summary is:

```text
=== Summary: 0 / 4 failed ===
```

To test the portable C reference implementation instead of the ASM implementation:

```bash
make clean
make test CONFIG=release USE_ASM=no
```

Run static analysis:

```bash
make lint
```

The test files are organized as follows:

| File | Scope |
|---|---|
| `tests/test_bignum_init_from_string.c` | Deterministic parsing, base, boundary, and error tests |
| `tests/test_bignum_init_from_string_extra.c` | Canaries, invalid inputs, reference equivalence, and fuzz checks |
| `tests/test_bignum_init_from_string_mt.c` | Concurrent independent-object checks |
| `tests/test_bignum_init_from_string_runner.c` | Integration smoke test |

The extended suite performs 100,000 reproducible cases with a local reference model. The reference model compares both status values and all `bignum_t` words for successful cases.

## Benchmarks

The benchmark sources are:

```text
benchmarks/bench_bignum_init_from_string.c
benchmarks/bench_bignum_init_from_string_mt.c
```

Each benchmark generates deterministic decimal strings and reports the data mode, seed, fingerprint, checksum, successful-call count, elapsed time, and nanoseconds per call.

| Mode | Input pattern | Purpose |
|---|---|---|
| `all_zero` | Every supplied digit is `0` | Measures the zero-result path and repeated multiplication by the selected base |
| `all_nonzero` | Every supplied digit is `1` | Measures the regular successful parse path without invalid digits |
| `mixed` | Deterministic alternating and pseudo-random decimal digits | Measures a representative mixed workload and branch behavior |

### Single-thread CLI

```text
bin/bench_bignum_init_from_string \
  [--iterations N] \
  [--warmup N] \
  [--data-count N] \
  [--src-len N] \
  [--seed N] \
  [--data-mode all_zero|all_nonzero|mixed]
```

Example:

```bash
./bin/bench_bignum_init_from_string \
  --iterations 1000000 \
  --warmup 10000 \
  --data-count 8192 \
  --src-len 32 \
  --seed 0x9e3779b97f4a7c15 \
  --data-mode mixed
```

### Multithread CLI

```text
bin/bench_bignum_init_from_string_mt \
  [--threads N] \
  [--iterations N|--total-iterations N] \
  [--warmup N] \
  [--data-count N] \
  [--src-len N] \
  [--seed N] \
  [--data-mode all_zero|all_nonzero|mixed]
```

`--iterations` is the per-thread workload. `--total-iterations` is accepted for compatibility with the template workflow and should be chosen as a multiple of `--threads` when comparing runs.

For a fair one-thread/two-thread comparison, hold total work constant:

```bash
./bin/bench_bignum_init_from_string_mt \
  --threads 1 \
  --total-iterations 3200000 \
  --src-len 32 \
  --data-mode mixed

./bin/bench_bignum_init_from_string_mt \
  --threads 2 \
  --total-iterations 3200000 \
  --src-len 32 \
  --data-mode mixed
```

## Perf workflow

The current environment provides two logical CPUs. The corresponding MT settings are:

```make
MT_THREADS=2
MT_CPU_LIST=0-1
MT_TOTAL_ITERATIONS=3200000
```

Run the complete ST/MT workflow for the supported data modes:

```bash
make bench_full CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=7 \
  KEEP_PERF=1
```

For targeted repeated counter measurements:

```bash
make bench_stat_st CONFIG=release \
  REPORT_NAME=baseline_st_mixed \
  DATA_MODE=mixed \
  PERF_RUNS=7

make bench_stat_mt CONFIG=release \
  REPORT_NAME=baseline_mt_mixed \
  DATA_MODE=mixed \
  MT_THREADS=2 \
  MT_CPU_LIST=0-1 \
  MT_TOTAL_ITERATIONS=3200000 \
  PERF_RUNS=7
```

Reports are written to `benchmarks/reports/`. With `KEEP_PERF=1`, raw profiles are retained as `.perf.data` files. Runtime validation checks the dynamic benchmark identifier generated from `LIB_NAME`, the selected data mode, and the elapsed-time field.

A reproducible optimization comparison should keep `CONFIG`, `PERF_RUNS`, `DATA_MODE`, input length, seed, thread count, CPU affinity, and total iterations constant:

```bash
make clean
make test CONFIG=release
make bench_full CONFIG=release REPORT_NAME=baseline PERF_RUNS=7 KEEP_PERF=1

# Change implementation, then repeat the verification.
make clean
make test CONFIG=release
make bench_full CONFIG=release REPORT_NAME=opt_v1 PERF_RUNS=7 KEEP_PERF=1
```

Compare matching reports only:

```bash
diff -u \
  benchmarks/reports/baseline_all_nonzero_st_stat.csv \
  benchmarks/reports/opt_v1_all_nonzero_st_stat.csv
```

## Installation and distribution

Build the object-file distribution:

```bash
make install CONFIG=release
```

Build the single-header and static-library distribution:

```bash
make dist CONFIG=release
```

Remove generated artifacts:

```bash
make clean
```

The distribution contains `bignum_init_from_string.h`, the bundled `bignum-core` declarations, the object file or static library, the README, the license, and the integration runner. The `dist` smoke test compiles and runs the generated runner against the distribution.

## Linking the object file

```bash
make build CONFIG=release

gcc your_app.c \
  build/bignum_init_from_string.o \
  -I./include \
  -I./libs/bignum-core/include \
  -o your_app \
  -no-pie
```

The application must use the same System V AMD64 ABI and include the `bignum_t` definition supplied by `bignum-core`.

## Contributing

Contributions should preserve the typed status contract, the documented base and prefix rules, the no-external-call requirement of the implementation, and the unspecified-on-error destination rule. New behavior must include deterministic tests and, where appropriate, reference-model fuzz coverage. Every change should run both `make test CONFIG=release`, `make test CONFIG=release USE_ASM=no`, and `make lint`. Performance changes should include reproducible benchmark parameters and matching ST/MT evidence.

The Makefile is part of the repository template and must not be modified without direct authorization.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
