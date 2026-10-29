/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_ROOTFS_H
#define KS_ROOTFS_H

#define KS_NEWROOT "/newroot"

/** Mount /dev, /proc, /sys and /run in the initramfs and open the console */
int ks_early_mounts(void);

/** Mount the root file system read only at KS_NEWROOT */
int ks_mount_root(const char *device, const char *fstype, const char *options);

/**
 * Move /dev, /proc, /sys and /run into KS_NEWROOT, delete the files of the
 * initramfs, make KS_NEWROOT the root and execute 'init'. Only returns on
 * errors.
 *
 * @param[in] init Init of the new root
 * @param[in] argv Arguments, argv[0] is replaced with 'init'
 */
int ks_switch_root(const char *init, char **argv);

#endif
