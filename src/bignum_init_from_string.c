/* bignum_init_from_string.c: переносимая reference-реализация. */
/* ------------------------------------------------------------------ */
#include <stddef.h>
#include <stdint.h>

#include "bignum_init_from_string.h"

static int valid_base(int base)
{
    return (base >= 2 && base <= 16) || base == 0;
}

static int ascii_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '\v' || c == '\f';
}

static int digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return (int)(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return (int)(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return (int)(c - 'A' + 10);
    }
    return -1;
}

static void consume_ignored_sign(const char **str)
{
    if (**str == '+' || **str == '-') {
        ++*str;
    }
}

static void consume_prefix(const char **str, int base, int *effective_base)
{
    const char *s = *str;

    if (s[0] != '0') {
        return;
    }
    if ((base == 0 || base == 2) &&
        (s[1] == 'b' || s[1] == 'B')) {
        *effective_base = 2;
        *str += 2;
        return;
    }
    if ((base == 0 || base == 8) &&
        (s[1] == 'o' || s[1] == 'O')) {
        *effective_base = 8;
        *str += 2;
        return;
    }
    if ((base == 0 || base == 16) &&
        (s[1] == 'x' || s[1] == 'X')) {
        *effective_base = 16;
        *str += 2;
    }
}

static void clear_words(bignum_t *x)
{
    size_t i;

    for (i = 0; i < BIGNUM_CAPACITY; ++i) {
        x->words[i] = UINT64_C(0);
    }
    x->len = 0U;
}

static int append_digit_wide(bignum_t *acc, uint64_t base, uint64_t digit)
{
    size_t i;
    uint64_t carry;

    carry = UINT64_C(0);
    for (i = 0; i < acc->len; ++i) {
#if defined(__GNUC__) || defined(__clang__)
        __uint128_t product = (__uint128_t)acc->words[i] * base + carry;
        acc->words[i] = (uint64_t)product;
        carry = (uint64_t)(product >> 64);
#else
        uint64_t word = acc->words[i];
        uint64_t low = (word & UINT64_C(0xFFFFFFFF)) * base + carry;
        uint64_t high = (word >> 32) * base + (low >> 32);

        acc->words[i] = (low & UINT64_C(0xFFFFFFFF)) |
                        (high << 32);
        carry = high >> 32;
#endif
    }

    if (carry != UINT64_C(0)) {
        if (acc->len >= BIGNUM_CAPACITY) {
            return 1;
        }
        acc->words[acc->len] = carry;
        ++acc->len;
    }

    carry = digit;
    for (i = 0; i < acc->len && carry != UINT64_C(0); ++i) {
        uint64_t old_word = acc->words[i];
        uint64_t next = old_word + carry;

        acc->words[i] = next;
        carry = next < old_word ? UINT64_C(1) : UINT64_C(0);
    }

    if (carry != UINT64_C(0)) {
        if (acc->len >= BIGNUM_CAPACITY) {
            return 1;
        }
        acc->words[acc->len] = carry;
        ++acc->len;
    }

    return 0;
}

static int append_shift_digit(bignum_t *acc, unsigned shift, uint64_t digit)
{
    size_t i;
    uint64_t carry = digit;

    for (i = 0; i < acc->len; ++i) {
        uint64_t word = acc->words[i];
        uint64_t next = (word << shift) | carry;

        carry = word >> (64U - shift);
        acc->words[i] = next;
    }

    if (carry != UINT64_C(0)) {
        if (acc->len >= BIGNUM_CAPACITY) {
            return 1;
        }
        acc->words[acc->len++] = carry;
    }
    return 0;
}

static int append_digit(bignum_t *acc, uint64_t base, uint64_t digit)
{
    if (base == UINT64_C(2)) {
        return append_shift_digit(acc, 1U, digit);
    }
    if (base == UINT64_C(8)) {
        return append_shift_digit(acc, 3U, digit);
    }
    if (base == UINT64_C(16)) {
        return append_shift_digit(acc, 4U, digit);
    }
    return append_digit_wide(acc, base, digit);
}

bignum_init_from_string_status_t bignum_init_from_string(bignum_t *dst,
                                        const char *str,
                                        int base)
{
    bignum_t acc;
    size_t i;
    int effective_base;

    if (dst == NULL || str == NULL) {
        return BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG;
    }
    if (!valid_base(base)) {
        return BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE;
    }

    while (ascii_space(*str)) {
        ++str;
    }

    consume_ignored_sign(&str);
    while (ascii_space(*str)) {
        ++str;
    }

    effective_base = base == 0 ? 10 : base;
    consume_prefix(&str, base, &effective_base);

    if (*str == '\0') {
        return BIGNUM_INIT_FROM_STRING_ERROR_EMPTY;
    }

    clear_words(&acc);
    for (i = 0; str[i] != '\0'; ++i) {
        int digit = digit_value(str[i]);

        if (digit < 0 || digit >= effective_base) {
            return BIGNUM_INIT_FROM_STRING_ERROR_PARSE;
        }
        if (append_digit(&acc, (uint64_t)effective_base,
                         (uint64_t)digit) != 0) {
            return BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW;
        }
    }

    for (i = 0; i < BIGNUM_CAPACITY; ++i) {
        dst->words[i] = acc.words[i];
    }
    dst->len = acc.len;
    return BIGNUM_INIT_FROM_STRING_SUCCESS;
}
