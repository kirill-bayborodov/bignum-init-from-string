/* Multithreaded independent-object tests for bignum_init_from_string. */
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_init_from_string.h"

typedef struct {
    const char *text;
    int base;
    uint64_t expected_word;
    size_t expected_len;
    int failed;
} worker_arg_t;

static void *worker(void *opaque)
{
    worker_arg_t *arg = (worker_arg_t *)opaque;
    bignum_t value;
    bignum_init_from_string_status_t status = bignum_init_from_string(&value, arg->text, arg->base);

    if (status != BIGNUM_INIT_FROM_STRING_SUCCESS || value.len != arg->expected_len ||
        value.words[0] != arg->expected_word) {
        arg->failed = 1;
    }
    return NULL;
}

int main(void)
{
    enum { THREADS = 8 };
    pthread_t threads[THREADS];
    worker_arg_t args[THREADS] = {
        { "12345", 10, UINT64_C(12345), 1U, 0 },
        { "DEADBEEF", 16, UINT64_C(0xDEADBEEF), 1U, 0 },
        { "0xFF", 0, UINT64_C(0xFF), 1U, 0 },
        { "42", 10, UINT64_C(42), 1U, 0 },
        { "ffff", 16, UINT64_C(0xFFFF), 1U, 0 },
        { "10000000000000000", 16, UINT64_C(0), 2U, 0 },
        { "7", 8, UINT64_C(7), 1U, 0 },
        { "101010", 2, UINT64_C(42), 1U, 0 }
    };

    for (int i = 0; i < THREADS; ++i) {
        assert(pthread_create(&threads[i], NULL, worker, &args[i]) == 0);
    }
    for (int i = 0; i < THREADS; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
        assert(args[i].failed == 0);
    }

    puts("--- Multithreaded bignum_init_from_string test passed ---");
    return 0;
}
