/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "chosen.h"
#include "common.h"
#include <errno.h>
#include <string.h>

static struct ks_config cfg;

static int setup(void **state)
{
    (void)state;
    test_config(&cfg);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    test_cleanup();
    return 0;
}

static void test_all_properties(void **state)
{
    (void)state;
    struct ks_chosen chosen;
    const uint32_t slc = KS_SLC_CONFIGURATION_LOCKED;
    const uint32_t active_key = 0xa90f9680;
    const uint32_t keys[] = { 0xa90f9680, 0x57f9c010, 0x12345678 };

    test_chosen_string("pb,active-system", "B");
    test_chosen_string("pb,device-uuid", "6f2c7c8e-6a4e-5b4c-8e2f-3c0d9a1b2c3d");
    test_chosen_cells("pb,slc", &slc, 1);
    test_chosen_cells("pb,slc-active-key", &active_key, 1);
    test_chosen_cells("pb,slc-available-keys", keys, 3);

    assert_int_equal(ks_chosen_read(cfg.chosen, &chosen), 0);
    assert_true(chosen.has_active_system);
    assert_string_equal(chosen.active_system, "B");
    assert_true(chosen.has_device_uuid);
    assert_string_equal(chosen.device_uuid, "6f2c7c8e-6a4e-5b4c-8e2f-3c0d9a1b2c3d");
    assert_true(chosen.has_slc);
    assert_int_equal(chosen.slc, KS_SLC_CONFIGURATION_LOCKED);
    assert_string_equal(ks_slc_name(chosen.slc), "configuration-locked");
    assert_true(chosen.has_active_key);
    assert_int_equal(chosen.active_key, 0xa90f9680);
    assert_true(chosen.has_available_keys);
    assert_int_equal(chosen.no_of_available_keys, 3);
    assert_int_equal(chosen.available_keys[1], 0x57f9c010);
    assert_int_equal(chosen.available_keys[2], 0x12345678);
}

static void test_missing_properties(void **state)
{
    (void)state;
    struct ks_chosen chosen;

    assert_int_equal(ks_chosen_read(cfg.chosen, &chosen), 0);
    assert_false(chosen.has_active_system);
    assert_false(chosen.has_device_uuid);
    assert_false(chosen.has_slc);
    assert_false(chosen.has_active_key);
    assert_false(chosen.has_available_keys);
}

static void test_missing_node(void **state)
{
    (void)state;
    struct ks_chosen chosen;

    assert_int_equal(ks_chosen_read("/nonexistent/chosen", &chosen), -ENOENT);
}

static void test_bad_cell_size(void **state)
{
    (void)state;
    struct ks_chosen chosen;

    test_write_file("chosen/pb,slc", "\0\0\0", 3);
    assert_int_equal(ks_chosen_read(cfg.chosen, &chosen), -EINVAL);
}

static void test_too_many_cells(void **state)
{
    (void)state;
    struct ks_chosen chosen;
    const uint32_t slc[2] = { 1, 2 };

    test_chosen_cells("pb,slc", slc, 2);
    assert_int_equal(ks_chosen_read(cfg.chosen, &chosen), -EINVAL);
}

static void test_unterminated_string(void **state)
{
    (void)state;
    struct ks_chosen chosen;

    test_write_file("chosen/pb,active-system", "A", 1);
    assert_int_equal(ks_chosen_read(cfg.chosen, &chosen), -EINVAL);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_all_properties, setup, teardown),
        cmocka_unit_test_setup_teardown(test_missing_properties, setup, teardown),
        cmocka_unit_test_setup_teardown(test_missing_node, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_cell_size, setup, teardown),
        cmocka_unit_test_setup_teardown(test_too_many_cells, setup, teardown),
        cmocka_unit_test_setup_teardown(test_unterminated_string, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
