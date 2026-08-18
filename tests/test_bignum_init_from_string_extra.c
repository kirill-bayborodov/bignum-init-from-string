/* Extended deterministic and fuzz/reference tests for bignum_init_from_string. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bignum_init_from_string.h"

static uint64_t next_random(uint64_t *state)
{
    uint64_t x = *state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static int reference_parse(bignum_t *out, const char *str, int base)
{
    size_t i;
    int effective_base = base;

    for (i = 0; i < BIGNUM_CAPACITY; ++i) {
        out->words[i] = UINT64_C(0);
    }
    out->len = 0U;

    while (*str == ' ' || *str == '\t' || *str == '\n' ||
           *str == '\r' || *str == '\v' || *str == '\f') {
        ++str;
    }
    if (*str == '+' || *str == '-') {
        ++str;
        while (*str == ' ' || *str == '\t' || *str == '\n' ||
               *str == '\r' || *str == '\v' || *str == '\f') {
            ++str;
        }
    }

    effective_base = base == 0 ? 10 : base;
    if (str[0] == '0' &&
        (base == 0 || base == 2) &&
        (str[1] == 'b' || str[1] == 'B')) {
        str += 2;
        effective_base = 2;
    } else if (str[0] == '0' &&
               (base == 0 || base == 8) &&
               (str[1] == 'o' || str[1] == 'O')) {
        str += 2;
        effective_base = 8;
    } else if (str[0] == '0' &&
               (base == 0 || base == 16) &&
               (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        effective_base = 16;
    }

    if (*str == '\0') {
        return BIGNUM_INIT_FROM_STRING_ERROR_EMPTY;
    }

    for (i = 0; str[i] != '\0'; ++i) {
        int digit;
        uint64_t carry;

        if (str[i] >= '0' && str[i] <= '9') {
            digit = (int)(str[i] - '0');
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            digit = (int)(str[i] - 'a' + 10);
        } else if (str[i] >= 'A' && str[i] <= 'F') {
            digit = (int)(str[i] - 'A' + 10);
        } else {
            return BIGNUM_INIT_FROM_STRING_ERROR_PARSE;
        }
        if (digit >= effective_base) {
            return BIGNUM_INIT_FROM_STRING_ERROR_PARSE;
        }

        carry = UINT64_C(0);
        for (size_t j = 0; j < out->len; ++j) {
            __uint128_t product = (__uint128_t)out->words[j] *
                                  (uint64_t)effective_base + carry;
            out->words[j] = (uint64_t)product;
            carry = (uint64_t)(product >> 64);
        }
        if (carry != UINT64_C(0)) {
            if (out->len >= BIGNUM_CAPACITY) {
                return BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW;
            }
            out->words[out->len++] = carry;
        }

        carry = (uint64_t)digit;
        for (size_t j = 0; j < out->len && carry != UINT64_C(0); ++j) {
            uint64_t old = out->words[j];
            out->words[j] += carry;
            carry = out->words[j] < old ? UINT64_C(1) : UINT64_C(0);
        }
        if (carry != UINT64_C(0)) {
            if (out->len >= BIGNUM_CAPACITY) {
                return BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW;
            }
            out->words[out->len++] = carry;
        }
    }

    return BIGNUM_INIT_FROM_STRING_SUCCESS;
}

static void assert_equal(const bignum_t *left, const bignum_t *right)
{
    assert(left->len == right->len);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(left->words[i] == right->words[i]);
    }
}

static void test_canaries(void)
{
    struct {
        uint64_t before;
        bignum_t value;
        uint64_t after;
    } boxed;

    boxed.before = UINT64_C(0x1122334455667788);
    boxed.after = UINT64_C(0x8877665544332211);
    assert(bignum_init_from_string(&boxed.value, "  0xDEADBEEF", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(boxed.value.len == 1U);
    assert(boxed.value.words[0] == UINT64_C(0xDEADBEEF));
    assert(boxed.before == UINT64_C(0x1122334455667788));
    assert(boxed.after == UINT64_C(0x8877665544332211));
    puts("test_canaries: PASSED");
}

static void test_fuzz_reference(void)
{
    static const char alphabet[] = "0123456789abcdefABCDEF";
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    char text[48];

    for (size_t n = 0; n < 100000U; ++n) {
        bignum_t actual;
        bignum_t expected;
        int base = (int)(next_random(&state) % 15U) + 2;
        size_t length = (size_t)(next_random(&state) % 24U) + 1U;

        if ((n % 7U) == 0U) {
            base = 16;
            text[0] = '0';
            text[1] = 'x';
            for (size_t i = 2; i < length + 2U; ++i) {
                text[i] = alphabet[next_random(&state) % 22U];
            }
            text[length + 2U] = '\0';
        } else {
            for (size_t i = 0; i < length; ++i) {
                text[i] = alphabet[next_random(&state) % 22U];
            }
            text[length] = '\0';
        }

        int expected_status = reference_parse(&expected, text, base);
        int actual_status = bignum_init_from_string(&actual, text, base);
        assert(actual_status == expected_status);
        if (actual_status == BIGNUM_INIT_FROM_STRING_SUCCESS) {
            assert_equal(&actual, &expected);
        }
    }
    puts("test_fuzz_reference: PASSED (100000 cases)");
}

static void test_prefixes_and_ignored_signs(void)
{
    bignum_t value;

    assert(bignum_init_from_string(&value, "0b101010", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));
    assert(bignum_init_from_string(&value, "0B101010", 2) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));

    assert(bignum_init_from_string(&value, "0o52", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));
    assert(bignum_init_from_string(&value, "0O52", 8) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));

    assert(bignum_init_from_string(&value, "+0x2A", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));
    assert(bignum_init_from_string(&value, "-  0x2A", 16) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U && value.words[0] == UINT64_C(42));

    assert(bignum_init_from_string(&value, "+", 10) ==
           BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&value, "-  ", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&value, "++1", 10) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&value, "0x10", 8) ==
           BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    puts("test_prefixes_and_ignored_signs: PASSED");
}

static void test_invalid_inputs(void)
{
    bignum_t value;

    assert(bignum_init_from_string(&value, "123z", 10) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&value, "0x", 0) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&value, "", 0) == BIGNUM_INIT_FROM_STRING_ERROR_EMPTY);
    assert(bignum_init_from_string(&value, "10", 1) == BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE);
    assert(bignum_init_from_string(NULL, "10", 10) == BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG);
    assert(bignum_init_from_string(&value, NULL, 10) == BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG);
    puts("test_invalid_inputs: PASSED");
}

static void test_carry_addition_overflow(void)
{
    /* This is ((2^2048 - 1) / 3) followed by digit 1 in base 3.
     * The multiplication reaches all 32 words, then adding 1 produces
     * carry beyond capacity and exercises append_digit line 98. */
    static const char text[] =
        "101112200200221221202211101220010200002212121111120112120101202021000102120021222220120112121120011210101120220020000222102122122221022221101212112111221210022202000121002220000022020111120101211200221102022111011112011221122020100201221101120020202110122110000121000020200011102020210022202000101002120210100211211111202002001211112112222111002201011111120012022211002221202002121112121021022202211210211011110022211120101111101001101102210221211111222221010210210211020012100021111022020010220021202011122002110202110201022200200210012002102202212002102221001212020000002020112112110211102110011221110122222001120112021221022000011112011012021222111001110001222222012220202020201000022001002222112011102112120201012212022111211212000201201010210111201212000121022100200110210112102121100121002120120111110211111010121012221220011010212211102001220101022121220100100021011012210220201120120012200211100202002200200010212222001000120102101200222010001002111120210211020210210102020101010222022201120212001112102012111121220221112122211211222012001120022101022020102100222102121002220121022022112222222212002121211101011220101200212010221011221110012021002110222212110201012112220000011010210020110000020012222112202102200201212020220002002211120010022222121220221022212212000210100210110122211";

    bignum_t value;
    assert(bignum_init_from_string(&value, text, 3) == BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW);

    {
        char multiply_overflow[sizeof(text) + 1U];

        memcpy(multiply_overflow, text, sizeof(text));
        multiply_overflow[sizeof(text) - 2U] = '0';
        multiply_overflow[sizeof(text) - 1U] = '0';
        multiply_overflow[sizeof(text)] = '\0';
        assert(bignum_init_from_string(&value, multiply_overflow, 3) ==
               BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW);
    }
    puts("test_carry_addition_overflow: PASSED");
}

int main(void)
{
    puts("--- Starting extended bignum_init_from_string tests ---");
    test_canaries();
    test_prefixes_and_ignored_signs();
    test_invalid_inputs();
    test_carry_addition_overflow();
    test_fuzz_reference();
    puts("--- All extended bignum_init_from_string tests passed ---");
    return 0;
}
