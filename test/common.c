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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Generated from the keystore of mkfixtures.sh */
extern const struct bpak_keystore keystore_kickstart;

static char tmpdir[256];
static char chosen_dir[300];

const char *test_tmpdir(void)
{
    if (tmpdir[0] == 0) {
        const char *base = getenv("TMPDIR");

        snprintf(tmpdir, sizeof(tmpdir), "%s/kickstart-test-XXXXXX", base ? base : "/tmp");
        assert_non_null(mkdtemp(tmpdir));
        snprintf(chosen_dir, sizeof(chosen_dir), "%s/chosen", tmpdir);
        assert_int_equal(mkdir(chosen_dir, 0755), 0);
    }

    return tmpdir;
}

void test_cleanup(void)
{
    char cmd[300];

    if (tmpdir[0] == 0)
        return;

    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
    assert_int_equal(system(cmd), 0);
    tmpdir[0] = 0;
}

void test_config(struct ks_config *cfg)
{
    test_tmpdir();

    memset(cfg, 0, sizeof(*cfg));
    cfg->root[KS_SYSTEM_A] = FIXTURE("rootfs.bpak");
    cfg->root[KS_SYSTEM_B] = FIXTURE("rootfs-unaligned.bpak");
    cfg->root_part = "rootfs";
    cfg->root_fstype = "squashfs";
    cfg->root_options = "";
    cfg->chosen = chosen_dir;
    cfg->init = "/sbin/init";
    cfg->device_timeout_ms = 100;
    cfg->verity_mode = KS_VERITY_RESTART;
    cfg->require_pb_keys = true;
    cfg->log_level = KS_LOG_DBG;
    cfg->keystore = &keystore_kickstart;
}

const char *test_write_file(const char *name, const void *data, size_t size)
{
    static char path[512];
    FILE *fp;

    snprintf(path, sizeof(path), "%s/%s", test_tmpdir(), name);
    fp = fopen(path, "wb");
    assert_non_null(fp);
    assert_int_equal(fwrite(data, 1, size, fp), size);
    fclose(fp);

    return path;
}

void test_chosen_string(const char *name, const char *value)
{
    char path[300];

    snprintf(path, sizeof(path), "chosen/%s", name);
    test_write_file(path, value, strlen(value) + 1);
}

void test_chosen_cells(const char *name, const uint32_t *cells, size_t count)
{
    char path[300];
    uint8_t buf[256];

    assert_true(count * 4 <= sizeof(buf));

    for (size_t i = 0; i < count; i++) {
        buf[i * 4] = cells[i] >> 24;
        buf[i * 4 + 1] = cells[i] >> 16;
        buf[i * 4 + 2] = cells[i] >> 8;
        buf[i * 4 + 3] = cells[i];
    }

    snprintf(path, sizeof(path), "chosen/%s", name);
    test_write_file(path, buf, count * 4);
}

void test_pbstate(struct pb_boot_state *state, uint32_t enable, uint32_t verified)
{
    memset(state, 0, sizeof(*state));
    state->magic = PB_STATE_MAGIC;
    state->enable = enable;
    state->verified = verified;
    state->remaining_boot_attempts = 3;
    state->board_regs[PB_STATE_NO_OF_BOARD_REGS - 1] = 0x11111111;
    state->crc = ks_crc32_update(0, state, sizeof(*state));
}
