/* bignum_init_from_string deterministic tests. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bignum_init_from_string.h"

static void check_zero(const bignum_t *b)
{
    size_t i;

    assert(b->len == 0U);
    for (i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b->words[i] == UINT64_C(0));
    }
}

static void check_value(const bignum_t *b, size_t len,
                        uint64_t word0, uint64_t word1)
{
    assert(b->len == len);
    assert(b->words[0] == word0);
    if (len > 1U) {
        assert(b->words[1] == word1);
    }
    for (size_t i = len; i < BIGNUM_CAPACITY; ++i) {
        assert(b->words[i] == UINT64_C(0));
    }
}

static void test_decimal(void)
{
    bignum_t b;

    assert(bignum_init_from_string(&b, "0", 10) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_zero(&b);
    assert(bignum_init_from_string(&b, "12345", 10) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(12345), UINT64_C(0));
    assert(bignum_init_from_string(&b, "18446744073709551615", 10) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_MAX, UINT64_C(0));
    puts("test_decimal: PASSED");
}

static void test_hexadecimal(void)
{
    bignum_t b;

    assert(bignum_init_from_string(&b, "0", 16) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_zero(&b);
    assert(bignum_init_from_string(&b, "DEADBEEF", 16) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(0xDEADBEEF), UINT64_C(0));
    assert(bignum_init_from_string(&b, "0xFf", 16) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(0xFF), UINT64_C(0));
    puts("test_hexadecimal: PASSED");
}

static void test_auto_base_and_boundaries(void)
{
    bignum_t b;
    char full[513];
    char overflow[514];

    assert(bignum_init_from_string(&b, "0xFf", 0) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(0xFF), UINT64_C(0));
    assert(bignum_init_from_string(&b, "255", 0) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(255), UINT64_C(0));
    assert(bignum_init_from_string(&b, "10000000000000000", 16) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 2U, UINT64_C(0), UINT64_C(1));

    memset(full, 'F', sizeof(full) - 1U);
    full[sizeof(full) - 1U] = '\0';
    assert(bignum_init_from_string(&b, full, 16) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(b.len == BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == UINT64_MAX);
    }

    memset(overflow, 'F', sizeof(overflow) - 1U);
    overflow[sizeof(overflow) - 1U] = '\0';
    assert(bignum_init_from_string(&b, overflow, 16) ==
           BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW);
    puts("test_auto_base_and_boundaries: PASSED");
}

static void test_binary(void)
{
    bignum_t b;
    char max_binary[BIGNUM_CAPACITY * 64U + 1U];
    char overflow_binary[BIGNUM_CAPACITY * 64U + 2U];

    assert(bignum_init_from_string(&b, "0", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_zero(&b);

    assert(bignum_init_from_string(&b, "1", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(1), UINT64_C(0));

    assert(bignum_init_from_string(&b,
                                   "1111111111111111111111111111111111111111111111111111111111111111",
                                   2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_MAX, UINT64_C(0));

    for (size_t i = 0; i < sizeof(max_binary) - 1U; ++i) {
        max_binary[i] = '1';
    }
    max_binary[sizeof(max_binary) - 1U] = '\0';

    assert(bignum_init_from_string(&b, max_binary, 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(b.len == BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == UINT64_MAX);
    }

    for (size_t i = 0; i < sizeof(overflow_binary) - 1U; ++i) {
        overflow_binary[i] = '1';
    }
    overflow_binary[sizeof(overflow_binary) - 1U] = '\0';

    assert(bignum_init_from_string(&b, overflow_binary, 2) ==
           BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW);

    assert(bignum_init_from_string(&b, "101010", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(42), UINT64_C(0));

    assert(bignum_init_from_string(&b, "00000101", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(5), UINT64_C(0));

    puts("test_binary: PASSED");
}

static void test_octal(void)
{
    bignum_t b;
    char max_octal[683U + 1U];
    char overflow_octal[684U + 1U];

    assert(bignum_init_from_string(&b, "0", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_zero(&b);

    assert(bignum_init_from_string(&b, "7", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(7), UINT64_C(0));

    assert(bignum_init_from_string(&b, "10", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(8), UINT64_C(0));

    assert(bignum_init_from_string(&b, "1777777777777777777777", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0));

    max_octal[0] = '3';
    for (size_t i = 1; i < sizeof(max_octal) - 1U; ++i) {
        max_octal[i] = '7';
    }
    max_octal[sizeof(max_octal) - 1U] = '\0';

    assert(bignum_init_from_string(&b, max_octal, 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(b.len == BIGNUM_CAPACITY);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(b.words[i] == UINT64_MAX);
    }

    overflow_octal[0] = '1';
    for (size_t i = 1; i < sizeof(overflow_octal) - 1U; ++i) {
        overflow_octal[i] = '0';
    }
    overflow_octal[sizeof(overflow_octal) - 1U] = '\0';

    assert(bignum_init_from_string(&b, overflow_octal, 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW);

    assert(bignum_init_from_string(&b, "17", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(15), UINT64_C(0));

    assert(bignum_init_from_string(&b, "00017", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(15), UINT64_C(0));

    puts("test_octal: PASSED");
}

static void test_binary_and_octal_errors(void)
{
    bignum_t b;

    assert(bignum_init_from_string(&b, "2", 2) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "10201", 2) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0b101", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(5), UINT64_C(0));
    assert(bignum_init_from_string(&b, "0B101", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(5), UINT64_C(0));
    assert(bignum_init_from_string(&b, "+1", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(1), UINT64_C(0));
    assert(bignum_init_from_string(&b, "-1", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(1), UINT64_C(0));
    assert(bignum_init_from_string(&b, "10 01", 2) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);

    assert(bignum_init_from_string(&b, "8", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "19", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0o17", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(15), UINT64_C(0));
    assert(bignum_init_from_string(&b, "0O17", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(15), UINT64_C(0));
    assert(bignum_init_from_string(&b, "a7", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "17 7", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);

    assert(bignum_init_from_string(&b, "   ", 2) ==
           BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&b, "\t\n", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);

    puts("test_binary_and_octal_errors: PASSED");
}

static void test_other_supported_bases(void)
{
    bignum_t b;

    for (int base = 3; base <= 15; ++base) {
        assert(bignum_init_from_string(&b, "10", base) ==
               BIGNUM_INIT_FROM_STRING_SUCCESS);
        check_value(&b, 1U, (uint64_t)base, UINT64_C(0));
    }

    assert(bignum_init_from_string(&b, "102012", 3) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(302), UINT64_C(0));

    assert(bignum_init_from_string(&b, "A", 11) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(10), UINT64_C(0));
    assert(bignum_init_from_string(&b, "E", 15) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(14), UINT64_C(0));
    puts("test_other_supported_bases: PASSED");
}

static void test_errors_and_whitespace(void)
{
    bignum_t b;

    assert(bignum_init_from_string(&b, "", 10) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&b, "0x", 16) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&b, "12a", 10) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0x10", 10) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0b102", 0) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0o18", 0) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&b, "0b", 0) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&b, "0o", 8) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&b, "  \t42", 10) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(42), UINT64_C(0));
    assert(bignum_init_from_string(&b, "-  0x2A", 0) == BIGNUM_INIT_FROM_STRING_SUCCESS);
    check_value(&b, 1U, UINT64_C(42), UINT64_C(0));
    assert(bignum_init_from_string(&b, NULL, 10) == BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG);
    assert(bignum_init_from_string(NULL, "1", 10) == BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG);
    assert(bignum_init_from_string(&b, "10", 1) == BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE);
    assert(bignum_init_from_string(&b, "10", 17) == BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE);
    puts("test_errors_and_whitespace: PASSED");
}

int main(void)
{
    puts("--- Starting deterministic bignum_init_from_string tests ---");
    test_decimal();
    test_hexadecimal();
    test_binary();
    test_octal();
    test_binary_and_octal_errors();
    test_other_supported_bases();
    test_auto_base_and_boundaries();
    test_errors_and_whitespace();
    puts("--- All deterministic bignum_init_from_string tests passed ---");
    return 0;
}
