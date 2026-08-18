/* Reproducible multithread benchmark for bignum_init_from_string. */
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <pthread.h>
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
    unsigned threads;
} options_t;

typedef struct {
    const options_t *options;
    const char *text;
    uint64_t successful;
    uint64_t checksum;
} worker_arg_t;

static double seconds_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static uint64_t parse_u64(const char *value)
{
    return strtoull(value, NULL, 0);
}

static void make_text(char *text, size_t length, uint64_t seed,
                      const char *mode)
{
    uint64_t state = seed;
    for (size_t i = 0; i < length; ++i) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        if (strcmp(mode, "all_zero") == 0) {
            text[i] = '0';
        } else if (strcmp(mode, "all_nonzero") == 0) {
            text[i] = '1';
        } else {
            text[i] = (i % 2U) == 0U ? '1' :
                      (char)('0' + (state % 10U));
        }
    }
    text[length] = '\0';
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
    options->threads = 2U;
    int total_specified = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            options->threads = (unsigned)parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            options->iterations = parse_u64(argv[++i]);
        } else if (strcmp(argv[i], "--total-iterations") == 0 && i + 1 < argc) {
            options->iterations = parse_u64(argv[++i]);
            total_specified = 1;
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
    if (total_specified) {
        if (options->threads == 0U || options->iterations == 0U ||
            options->iterations % options->threads != 0U) {
            return 1;
        }
        options->iterations /= options->threads;
    }
    return options->iterations == 0U || options->threads == 0U ||
           options->src_len == 0U || options->src_len > 128U ||
           options->base < 0 || options->base > 16;
}

static void *worker(void *opaque)
{
    worker_arg_t *arg = (worker_arg_t *)opaque;
    bignum_t result;
    uint64_t checksum = 0U;
    uint64_t successful = 0U;

    for (uint64_t i = 0; i < arg->options->iterations; ++i) {
        if (bignum_init_from_string(&result, arg->text, arg->options->base) == BIGNUM_INIT_FROM_STRING_SUCCESS) {
            ++successful;
            checksum ^= result.words[0] + i;
        }
    }
    arg->successful = successful;
    arg->checksum = checksum;
    return NULL;
}

int main(int argc, char **argv)
{
    options_t options;
    pthread_t *threads;
    worker_arg_t *args;
    char *text;
    uint64_t successful = 0U;
    uint64_t checksum = 0U;
    double start;
    double elapsed;

    if (parse_options(argc, argv, &options) != 0) {
        fprintf(stderr, "invalid benchmark options\n");
        return 2;
    }

    threads = calloc(options.threads, sizeof(*threads));
    args = calloc(options.threads, sizeof(*args));
    text = calloc((size_t)options.src_len + 1U, sizeof(*text));
    if (threads == NULL || args == NULL || text == NULL) {
        free(threads);
        free(args);
        free(text);
        return 2;
    }
    make_text(text, (size_t)options.src_len, options.seed, options.data_mode);

    for (unsigned t = 0; t < options.threads; ++t) {
        args[t].options = &options;
        args[t].text = text;
        for (uint64_t i = 0; i < options.warmup; ++i) {
            bignum_t warm;
            (void)bignum_init_from_string(&warm, text, options.base);
        }
    }

    start = seconds_now();
    for (unsigned t = 0; t < options.threads; ++t) {
        if (pthread_create(&threads[t], NULL, worker, &args[t]) != 0) {
            return 2;
        }
    }
    for (unsigned t = 0; t < options.threads; ++t) {
        pthread_join(threads[t], NULL);
        successful += args[t].successful;
        checksum ^= args[t].checksum;
    }
    elapsed = seconds_now() - start;

    printf("benchmark=bignum_init_from_string_mt base=%d data_mode=%s seed=%" PRIu64
           " fingerprint=%" PRIu64 " checksum=%" PRIu64
           " successful=%" PRIu64 " elapsed_seconds=%.9f ns_per_call=%.3f\n",
           options.base, options.data_mode, options.seed, options.seed ^ options.src_len,
           checksum, successful, elapsed,
           elapsed * 1000000000.0 /
               (double)(options.iterations * options.threads));

    free(threads);
    free(args);
    free(text);
    return successful == options.iterations * options.threads ? 0 : 1;
}
