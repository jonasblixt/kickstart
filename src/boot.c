/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "boot.h"
#include "log.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int ks_wait_for_device(const char *path, unsigned int timeout_ms)
{
    uint64_t start = ks_now_ms();
    bool logged = false;

    while (access(path, F_OK) != 0) {
        if (ks_now_ms() - start >= timeout_ms) {
            LOG_ERR("Timeout waiting for %s", path);
            return -ETIMEDOUT;
        }

        if (!logged) {
            LOG_INFO("Waiting for %s", path);
            logged = true;
        }

        ks_sleep_ms(10);
    }

    return 0;
}

static void log_chosen(const struct ks_chosen *chosen)
{
    char keys[KS_CHOSEN_MAX_KEYS * 9 + 1] = "";

    if (chosen->has_device_uuid)
        LOG_INFO("Device UUID: %s", chosen->device_uuid);

    if (chosen->has_slc)
        LOG_INFO("SLC: %s (%u)", ks_slc_name(chosen->slc), chosen->slc);

    if (chosen->has_active_key)
        LOG_INFO("Boot image key: %08x", chosen->active_key);

    for (size_t i = 0; i < chosen->no_of_available_keys; i++)
        snprintf(
            keys + strlen(keys), sizeof(keys) - strlen(keys), " %08x", chosen->available_keys[i]);

    if (chosen->has_available_keys)
        LOG_INFO("Available keys:%s", keys);
}

static int read_pbstate(const struct ks_config *cfg, struct ks_boot *boot)
{
    int rc;
    bool from_backup;
    const struct pb_boot_state *s = &boot->pbstate;

    if (cfg->pbstate_primary == NULL)
        return 0;

    rc = ks_wait_for_device(cfg->pbstate_primary, cfg->device_timeout_ms);

    if (rc != 0)
        return rc;

    rc = ks_wait_for_device(cfg->pbstate_backup, cfg->device_timeout_ms);

    if (rc != 0)
        return rc;

    rc = ks_pbstate_read(cfg->pbstate_primary, cfg->pbstate_backup, &boot->pbstate, &from_backup);

    if (rc != 0)
        return rc;

    boot->has_pbstate = true;

    LOG_INFO("pbstate%s: A %s%s, B %s%s, %u boot attempts left%s%s",
             from_backup ? " (backup)" : "",
             ks_pbstate_enabled(s, KS_SYSTEM_A) ? "enabled" : "disabled",
             ks_pbstate_verified(s, KS_SYSTEM_A) ? " verified" : "",
             ks_pbstate_enabled(s, KS_SYSTEM_B) ? "enabled" : "disabled",
             ks_pbstate_verified(s, KS_SYSTEM_B) ? " verified" : "",
             s->remaining_boot_attempts,
             (s->error & PB_STATE_ERROR_A_ROLLBACK) ? ", rolled back from A" : "",
             (s->error & PB_STATE_ERROR_B_ROLLBACK) ? ", rolled back from B" : "");

    LOG_DBG("Board registers: %08x %08x %08x %08x",
            ks_pbstate_board_reg(s, 0),
            ks_pbstate_board_reg(s, 1),
            ks_pbstate_board_reg(s, 2),
            ks_pbstate_board_reg(s, 3));

    return 0;
}

static int select_system(struct ks_boot *boot)
{
    const char *active = boot->chosen.active_system;

    /* punchboot has selected the system that it booted, it is in the device
     * tree */
    if (boot->chosen.has_active_system && strcmp(active, "A") == 0) {
        boot->system = KS_SYSTEM_A;
    } else if (boot->chosen.has_active_system && strcmp(active, "B") == 0) {
        boot->system = KS_SYSTEM_B;
    } else if (boot->has_pbstate) {
        bool a = ks_pbstate_enabled(&boot->pbstate, KS_SYSTEM_A);
        bool b = ks_pbstate_enabled(&boot->pbstate, KS_SYSTEM_B);

        if (a == b) {
            LOG_ERR("No pb,active-system and the pbstate has %s enabled", a ? "both" : "none");
            return -ENOENT;
        }

        LOG_WARN("No pb,active-system, using the enabled system of the pbstate");
        boot->system = a ? KS_SYSTEM_A : KS_SYSTEM_B;
    } else {
        LOG_ERR("Unknown active system");
        return -ENOENT;
    }

    if (boot->has_pbstate && !ks_pbstate_enabled(&boot->pbstate, boot->system))
        LOG_WARN("System %s is not enabled in the pbstate", ks_system_name(boot->system));

    LOG_INFO("Active system: %s%s",
             ks_system_name(boot->system),
             boot->has_pbstate && ks_pbstate_verified(&boot->pbstate, boot->system) ? " (verified)"
                                                                                    : "");

    return 0;
}

int ks_boot_prepare(const struct ks_config *cfg, struct ks_boot *boot)
{
    int rc;

    memset(boot, 0, sizeof(*boot));

    rc = ks_chosen_read(cfg->chosen, &boot->chosen);

    if (rc != 0) {
        /* Without /chosen the system can still be selected from the pbstate,
         * the key policy decides if the image can be verified */
        LOG_WARN("Could not read %s (%i)", cfg->chosen, rc);
        memset(&boot->chosen, 0, sizeof(boot->chosen));
    }

    log_chosen(&boot->chosen);

    rc = read_pbstate(cfg, boot);

    if (rc != 0)
        LOG_WARN("Could not read the pbstate (%i)", rc);

    rc = select_system(boot);

    if (rc != 0)
        return rc;

    boot->root_device = cfg->root[boot->system];

    if (boot->root_device == NULL || boot->root_device[0] == 0) {
        LOG_ERR("No root device for system %s", ks_system_name(boot->system));
        return -ENOENT;
    }

    rc = ks_wait_for_device(boot->root_device, cfg->device_timeout_ms);

    if (rc != 0)
        return rc;

    rc = ks_image_load(boot->root_device, cfg, &boot->chosen, &boot->image);

    if (rc != 0)
        return rc;

    LOG_INFO("Root %s: %llu bytes at %llu, hash tree %llu bytes at %llu",
             boot->root_device,
             (unsigned long long)boot->image.data_size,
             (unsigned long long)boot->image.data_offset,
             (unsigned long long)boot->image.tree_size,
             (unsigned long long)boot->image.tree_offset);

    return 0;
}
