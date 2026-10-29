/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "boot.h"
#include "common.h"
#include <bpak/id.h>
#include <errno.h>
#include <string.h>

static struct ks_config cfg;
static char primary[512];
static char backup[512];

static int setup(void **state)
{
    (void)state;
    const uint32_t keys[] = { bpak_id("test-key") };

    test_config(&cfg);
    test_chosen_cells("pb,slc-available-keys", keys, 1);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    test_cleanup();
    return 0;
}

static void use_pbstate(uint32_t enable)
{
    struct pb_boot_state s;

    test_pbstate(&s, enable, 0);
    strcpy(primary, test_write_file("primary", &s, sizeof(s)));
    strcpy(backup, test_write_file("backup", &s, sizeof(s)));
    cfg.pbstate_primary = primary;
    cfg.pbstate_backup = backup;
}

static void test_active_system_from_chosen(void **state)
{
    (void)state;
    struct ks_boot boot;

    test_chosen_string("pb,active-system", "B");

    assert_int_equal(ks_boot_prepare(&cfg, &boot), 0);
    assert_int_equal(boot.system, KS_SYSTEM_B);
    assert_string_equal(boot.root_device, FIXTURE("rootfs-unaligned.bpak"));
    assert_false(boot.has_pbstate);
    assert_int_equal(boot.image.data_offset, 4096 + 1024);
}

static void test_chosen_wins_over_pbstate(void **state)
{
    (void)state;
    struct ks_boot boot;

    test_chosen_string("pb,active-system", "A");
    use_pbstate(PB_STATE_B_ENABLED);

    assert_int_equal(ks_boot_prepare(&cfg, &boot), 0);
    assert_true(boot.has_pbstate);
    assert_int_equal(boot.system, KS_SYSTEM_A);
}

static void test_fallback_to_pbstate(void **state)
{
    (void)state;
    struct ks_boot boot;

    test_chosen_string("pb,active-system", "?");
    use_pbstate(PB_STATE_A_ENABLED);

    assert_int_equal(ks_boot_prepare(&cfg, &boot), 0);
    assert_int_equal(boot.system, KS_SYSTEM_A);
}

static void test_pbstate_ambiguous(void **state)
{
    (void)state;
    struct ks_boot boot;

    use_pbstate(PB_STATE_A_ENABLED | PB_STATE_B_ENABLED);
    assert_int_equal(ks_boot_prepare(&cfg, &boot), -ENOENT);
}

static void test_no_active_system(void **state)
{
    (void)state;
    struct ks_boot boot;

    assert_int_equal(ks_boot_prepare(&cfg, &boot), -ENOENT);
}

static void test_root_device_timeout(void **state)
{
    (void)state;
    struct ks_boot boot;

    test_chosen_string("pb,active-system", "A");
    cfg.root[KS_SYSTEM_A] = "/dev/nonexistent-kickstart-test";

    assert_int_equal(ks_boot_prepare(&cfg, &boot), -ETIMEDOUT);
}

static void test_bad_image(void **state)
{
    (void)state;
    struct ks_boot boot;

    test_chosen_string("pb,active-system", "A");
    cfg.root[KS_SYSTEM_A] = FIXTURE("rootfs-bad-header.bpak");

    assert_int_equal(ks_boot_prepare(&cfg, &boot), -BPAK_VERIFY_FAIL);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_active_system_from_chosen, setup, teardown),
        cmocka_unit_test_setup_teardown(test_chosen_wins_over_pbstate, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fallback_to_pbstate, setup, teardown),
        cmocka_unit_test_setup_teardown(test_pbstate_ambiguous, setup, teardown),
        cmocka_unit_test_setup_teardown(test_no_active_system, setup, teardown),
        cmocka_unit_test_setup_teardown(test_root_device_timeout, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_image, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
