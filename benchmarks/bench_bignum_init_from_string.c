/* Reproducible single-thread benchmark for bignum_init_from_string. */
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bignum_init_from_string.h"

typedef struct {
    uint64_t iterations;
    uint64_t warmup;
    uint64_t data_count;
    uint64_t src_len;
    uint64_t seed;
    const char *data_mode;
    int base;
} options_t;

static uint64_t next_random(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static double seconds_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void make_text(char *text, size_t length, uint64_t *state,
                      const char *mode)
{
    for (size_t i = 0; i < length; ++i) {
        if (strcmp(mode, "all_zero") == 0) {
            text[i] = '0';
        } else if (strcmp(mode, "all_nonzero") == 0) {
            text[i] = '1';
        } else {
            text[i] = (i % 2U) == 0U ? '1' :
                      (char)('0' + (next_random(state) % 10U));
        }
    }
    text[length] = '\0';
}

static uint64_t parse_u64(const char *value)
{
    return strtoull(value, NULL, 0);
}

static int parse_options(int argc, char **argv, options_t *options)
{
    options->iterations = 1000000U;
    options->warmup = 10000U;
    options->data_count = 1U;
    options->src_len = 32U;
    options->seed = UINT64_C(0x9e3779b97f4a7c15);
    options->data_mode = "mixed";
    options->base = 10;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            options->iterations = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            options->warmup = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--data-count") == 0 && i + 1 < argc) {
            options->data_count = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--src-len") == 0 && i + 1 < argc) {
            options->src_len = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            options->seed = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--data-mode") == 0 && i + 1 < argc) {
            options->data_mode = argv[++i];
        } else if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
            options->base = (int)parse_u64(argv[++i]);
        } else {
            return 1;
        }
    }
    return options->iterations == 0U || options->data_count == 0U ||
           options->src_len == 0U || options->src_len > 128U ||
           options->base < 0 || options->base > 16;
}

int main(int argc, char **argv)
{
    options_t options;
    char text[129];
    bignum_t result;
    uint64_t state;
    uint64_t checksum = 0U;
    uint64_t successful = 0U;
    double start;
    double elapsed;

    if (parse_options(argc, argv, &options) != 0) {
        fprintf(stderr, "invalid benchmark options\n");
        return 2;
    }

    state = options.seed;
    make_text(text, (size_t)options.src_len, &state, options.data_mode);
    for (uint64_t i = 0; i < options.warmup; ++i) {
        (void)bignum_init_from_string(&result, text, options.base);
    }

    start = seconds_now();
    for (uint64_t i = 0; i < options.iterations; ++i) {
        bignum_init_from_string_status_t status = bignum_init_from_string(&result, text, options.base);
        if (status == BIGNUM_INIT_FROM_STRING_SUCCESS) {
            ++successful;
            checksum ^= result.words[0] + i;
        }
    }
    elapsed = seconds_now() - start;

    printf("benchmark=bignum_init_from_string_st base=%d data_mode=%s seed=%" PRIu64
           " fingerprint=%" PRIu64 " checksum=%" PRIu64
           " successful=%" PRIu64 " elapsed_seconds=%.9f ns_per_call=%.3f\n",
           options.base, options.data_mode, options.seed, state, checksum, successful,
           elapsed, elapsed * 1000000000.0 / (double)options.iterations);
    return successful == options.iterations ? 0 : 1;
}
