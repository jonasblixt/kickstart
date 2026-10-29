/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_CHOSEN_H
#define KS_CHOSEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KS_CHOSEN_MAX_KEYS 32

/** Security life cycle of punchboot, pb,slc */
enum ks_slc {
    KS_SLC_INVALID,
    KS_SLC_NOT_CONFIGURED,
    KS_SLC_CONFIGURATION,
    KS_SLC_CONFIGURATION_LOCKED,
    KS_SLC_EOL,
};

/**
 * The properties that punchboot adds to /chosen in the device tree, see
 * boot_driver_linux_prepare() in punchboot's src/boot/linux.c
 */
struct ks_chosen {
    bool has_active_system;
    char active_system[8]; /*!< pb,active-system: "A", "B" or "?" */
    bool has_device_uuid;
    char device_uuid[40]; /*!< pb,device-uuid */
    bool has_slc;
    uint32_t slc; /*!< pb,slc */
    bool has_active_key;
    uint32_t active_key; /*!< pb,slc-active-key, key of the boot image */
    bool has_available_keys;
    size_t no_of_available_keys;
    uint32_t available_keys[KS_CHOSEN_MAX_KEYS]; /*!< pb,slc-available-keys, not revoked */
};

/**
 * Read the punchboot properties of the /chosen node at 'path', usually
 * /proc/device-tree/chosen. Missing properties are not errors, the has_*
 * members tell which ones were found.
 *
 * @return 0 on success, -ENOENT if 'path' does not exist or -EINVAL for a
 *         property with an invalid size
 */
int ks_chosen_read(const char *path, struct ks_chosen *chosen);

const char *ks_slc_name(uint32_t slc);

#endif
