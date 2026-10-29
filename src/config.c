/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "config.h"
#include <kickstart/build_config.h>
#include <string.h>

/* Generated from KICKSTART_KEYSTORE with 'bpak generate keystore' */
extern const struct bpak_keystore keystore_kickstart;

static const char *optional(const char *value)
{
    return value[0] ? value : NULL;
}

void ks_config_init(struct ks_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->root[KS_SYSTEM_A] = KS_CONFIG_ROOT_A;
    cfg->root[KS_SYSTEM_B] = KS_CONFIG_ROOT_B;
    cfg->root_part = KS_CONFIG_ROOT_PART;
    cfg->root_fstype = KS_CONFIG_ROOT_FSTYPE;
    cfg->root_options = KS_CONFIG_ROOT_OPTIONS;
    cfg->pbstate_primary = optional(KS_CONFIG_PBSTATE_PRIMARY);
    cfg->pbstate_backup = optional(KS_CONFIG_PBSTATE_BACKUP);
    cfg->chosen = KS_CONFIG_CHOSEN;
    cfg->init = KS_CONFIG_INIT;
    cfg->device_timeout_ms = KS_CONFIG_DEVICE_TIMEOUT_MS;
    cfg->verity_mode = KS_CONFIG_VERITY_MODE;
    cfg->require_pb_keys = KS_CONFIG_REQUIRE_PB_KEYS;
    cfg->log_level = KS_CONFIG_LOG_LEVEL;
    cfg->keystore = &keystore_kickstart;
}
