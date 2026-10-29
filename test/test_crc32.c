/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "common.h"
#include "crc32.h"

static void test_zlib_check_value(void **state)
{
    (void)state;
    assert_int_equal(ks_crc32("123456789", 9), 0xcbf43926);
}

static void test_without_inversion(void **state)
{
    (void)state;
    /* punchboot's crc32(0, ...), the check value without the initial and
     * final inversion */
    assert_int_equal(ks_crc32_update(0, "123456789", 9), 0x2dfd2d88);
    assert_int_equal(ks_crc32_update(0, "", 0), 0);
}

static void test_incremental(void **state)
{
    (void)state;
    uint32_t crc = ks_crc32_update(0, "1234", 4);

    assert_int_equal(ks_crc32_update(crc, "56789", 5), ks_crc32_update(0, "123456789", 9));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_zlib_check_value),
        cmocka_unit_test(test_without_inversion),
        cmocka_unit_test(test_incremental),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
