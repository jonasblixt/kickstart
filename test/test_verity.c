/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "common.h"
#include "dm.h"
#include <errno.h>
#include <string.h>

static void make_image(struct ks_image *image)
{
    memset(image, 0, sizeof(*image));
    image->data_size = 201 * 4096;
    for (int i = 0; i < 32; i++) {
        image->root_hash[i] = i;
        image->salt[i] = 0xff - i;
    }
}

static void test_table(void **state)
{
    (void)state;
    struct ks_image image;
    char table[512];

    make_image(&image);

    assert_int_equal(ks_verity_table(table, sizeof(table), "253:0", "253:1", &image, KS_VERITY_EIO),
                     0);
    assert_string_equal(table,
                        "1 253:0 253:1 4096 4096 201 0 sha256 "
                        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f "
                        "fffefdfcfbfaf9f8f7f6f5f4f3f2f1f0efeeedecebeae9e8e7e6e5e4e3e2e1e0");
}

static void test_corruption_modes(void **state)
{
    (void)state;
    struct ks_image image;
    char table[512];
    size_t len;

    make_image(&image);

    assert_int_equal(
        ks_verity_table(table, sizeof(table), "253:0", "253:1", &image, KS_VERITY_RESTART), 0);
    len = strlen(table);
    assert_string_equal(table + len - strlen(" 1 restart_on_corruption"),
                        " 1 restart_on_corruption");

    assert_int_equal(
        ks_verity_table(table, sizeof(table), "253:0", "253:1", &image, KS_VERITY_PANIC), 0);
    len = strlen(table);
    assert_string_equal(table + len - strlen(" 1 panic_on_corruption"), " 1 panic_on_corruption");
}

static void test_small_buffer(void **state)
{
    (void)state;
    struct ks_image image;
    char table[64];

    make_image(&image);
    assert_int_equal(ks_verity_table(table, sizeof(table), "253:0", "253:1", &image, KS_VERITY_EIO),
                     -ENOSPC);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_table),
        cmocka_unit_test(test_corruption_modes),
        cmocka_unit_test(test_small_buffer),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
