/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "common.h"
#include "pbstate.h"
#include <errno.h>
#include <string.h>

static char primary[512];
static char backup[512];

static int setup(void **state)
{
    (void)state;
    test_tmpdir();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    test_cleanup();
    return 0;
}

static void write_states(const struct pb_boot_state *p, const struct pb_boot_state *b)
{
    strcpy(primary, test_write_file("primary", p, sizeof(*p)));
    strcpy(backup, test_write_file("backup", b, sizeof(*b)));
}

static void test_primary(void **state)
{
    (void)state;
    struct pb_boot_state p, b, out;
    bool from_backup = true;

    test_pbstate(&p, PB_STATE_A_ENABLED, PB_STATE_A_VERIFIED);
    test_pbstate(&b, PB_STATE_B_ENABLED, 0);
    write_states(&p, &b);

    assert_int_equal(ks_pbstate_read(primary, backup, &out, &from_backup), 0);
    assert_false(from_backup);
    assert_true(ks_pbstate_enabled(&out, KS_SYSTEM_A));
    assert_false(ks_pbstate_enabled(&out, KS_SYSTEM_B));
    assert_true(ks_pbstate_verified(&out, KS_SYSTEM_A));
    assert_false(ks_pbstate_verified(&out, KS_SYSTEM_B));
    assert_int_equal(out.remaining_boot_attempts, 3);
    /* The board registers are stored in reverse order */
    assert_int_equal(ks_pbstate_board_reg(&out, 0), 0x11111111);
    assert_int_equal(ks_pbstate_board_reg(&out, 3), 0);
}

static void test_bad_primary_crc(void **state)
{
    (void)state;
    struct pb_boot_state p, b, out;
    bool from_backup = false;

    test_pbstate(&p, PB_STATE_A_ENABLED, 0);
    test_pbstate(&b, PB_STATE_B_ENABLED, 0);
    p.remaining_boot_attempts = 2; /* Changed after the checksum */
    write_states(&p, &b);

    assert_int_equal(ks_pbstate_read(primary, backup, &out, &from_backup), 0);
    assert_true(from_backup);
    assert_true(ks_pbstate_enabled(&out, KS_SYSTEM_B));
}

static void test_bad_primary_magic(void **state)
{
    (void)state;
    struct pb_boot_state p, b, out;

    test_pbstate(&p, PB_STATE_A_ENABLED, 0);
    test_pbstate(&b, PB_STATE_B_ENABLED, 0);
    p.magic = 0;
    write_states(&p, &b);

    assert_int_equal(ks_pbstate_check(&p), -EBADMSG);
    assert_int_equal(ks_pbstate_read(primary, backup, &out, NULL), 0);
    assert_true(ks_pbstate_enabled(&out, KS_SYSTEM_B));
}

static void test_both_bad(void **state)
{
    (void)state;
    struct pb_boot_state p, b, out;

    test_pbstate(&p, PB_STATE_A_ENABLED, 0);
    test_pbstate(&b, PB_STATE_B_ENABLED, 0);
    p.crc ^= 1;
    b.crc ^= 1;
    write_states(&p, &b);

    assert_int_equal(ks_pbstate_read(primary, backup, &out, NULL), -EBADMSG);
}

static void test_short_device(void **state)
{
    (void)state;
    struct pb_boot_state b, out;

    test_pbstate(&b, PB_STATE_B_ENABLED, 0);
    strcpy(primary, test_write_file("primary", &b, 100));
    strcpy(backup, test_write_file("backup", &b, 100));

    assert_int_equal(ks_pbstate_read(primary, backup, &out, NULL), -EIO);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_primary, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_primary_crc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_primary_magic, setup, teardown),
        cmocka_unit_test_setup_teardown(test_both_bad, setup, teardown),
        cmocka_unit_test_setup_teardown(test_short_device, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
