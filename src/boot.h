/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_BOOT_H
#define KS_BOOT_H

#include "chosen.h"
#include "config.h"
#include "image.h"
#include "pbstate.h"
#include <stdbool.h>

/** What kickstart found out before it changes anything */
struct ks_boot {
    struct ks_chosen chosen;
    bool has_pbstate;
    struct pb_boot_state pbstate;
    enum ks_system system;
    const char *root_device;
    struct ks_image image;
};

/**
 * Wait until 'path' exists, block devices show up while the kernel probes
 * the storage
 *
 * @return 0 when it exists or -ETIMEDOUT
 */
int ks_wait_for_device(const char *path, unsigned int timeout_ms);

/**
 * Read /chosen and the pbstate, select the system to boot and verify its
 * root image. Nothing is changed in the system, this is the dry run when
 * kickstart is not started as PID 1.
 *
 * @return 0 on success or a negative error code
 */
int ks_boot_prepare(const struct ks_config *cfg, struct ks_boot *boot);

#endif
