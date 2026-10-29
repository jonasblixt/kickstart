/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_CONFIG_H
#define KS_CONFIG_H

#include "log.h"
#include <bpak/keystore.h>
#include <stdbool.h>

/** What dm-verity does when it finds a corrupted block */
enum ks_verity_mode {
    KS_VERITY_EIO, /*!< Return I/O errors */
    KS_VERITY_RESTART, /*!< Restart the system, punchboot counts it as a failed boot */
    KS_VERITY_PANIC, /*!< Kernel panic */
};

enum ks_system {
    KS_SYSTEM_A,
    KS_SYSTEM_B,
    KS_SYSTEM_COUNT,
};

/**
 * The configuration of kickstart. It is fixed when kickstart is built, see
 * the KICKSTART_* options in CMakeLists.txt, ks_config_init() fills it in.
 */
struct ks_config {
    /** Block device with the root bpak of each system */
    const char *root[KS_SYSTEM_COUNT];
    /** Name of the part with the file system, its merkle tree is the part
     *  "<root_part>-hash-tree" */
    const char *root_part;
    const char *root_fstype;
    const char *root_options;
    /** Primary and backup pbstate block devices, NULL when not used */
    const char *pbstate_primary;
    const char *pbstate_backup;
    /** The /chosen node of the device tree that punchboot booted with */
    const char *chosen;
    /** Init of the root file system */
    const char *init;
    /** How long to wait for block devices to show up */
    unsigned int device_timeout_ms;
    enum ks_verity_mode verity_mode;
    /** Only accept root images that are signed with a key that punchboot
     *  lists in /chosen/pb,slc-available-keys, the keys that are not
     *  revoked, and fail when the property is missing */
    bool require_pb_keys;
    enum ks_log_level log_level;
    /** Keys that root images are signed with, compiled in */
    const struct bpak_keystore *keystore;
};

/** The configuration that kickstart was built with */
void ks_config_init(struct ks_config *cfg);

static inline const char *ks_system_name(enum ks_system system)
{
    return system == KS_SYSTEM_A ? "A" : "B";
}

#endif
