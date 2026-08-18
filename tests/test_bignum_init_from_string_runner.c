/* Integration smoke test for bignum_init_from_string. */
#include <assert.h>
#include <stdio.h>

#include "bignum_init_from_string.h"

int main(void)
{
    bignum_t value;

    printf("Running test: test_bignum_init_from_string_runner... ");
    assert(bignum_init_from_string(&value, "  123456789", 10) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 1U);
    assert(value.words[0] == UINT64_C(123456789));
    assert(bignum_init_from_string(&value, "0x10000000000000000", 0) ==
           BIGNUM_INIT_FROM_STRING_SUCCESS);
    assert(value.len == 2U);
    assert(value.words[0] == 0U && value.words[1] == 1U);
    assert(bignum_init_from_string(&value, "bad", 10) == BIGNUM_INIT_FROM_STRING_ERROR_PARSE);
    assert(bignum_init_from_string(&value, NULL, 10) == BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG);
    puts("PASSED");
    return 0;
}
