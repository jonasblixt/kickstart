/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_DM_H
#define KS_DM_H

#include "config.h"
#include "image.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * Create and activate a read only device mapper device with one target,
 * without a device node
 *
 * @param[in] name Device name, /dev/mapper/<name>
 * @param[in] target Target type, "linear" or "verity"
 * @param[in] sectors Size of the device in 512 byte sectors
 * @param[in] params Target parameters
 * @param[out] dev The new device
 *
 * @return 0 on success or -errno
 */
int ks_dm_create(const char *name,
                 const char *target,
                 uint64_t sectors,
                 const char *params,
                 dev_t *dev);

/** Remove the device mapper device 'name' */
int ks_dm_remove(const char *name);

/**
 * The dm-verity table of 'image' with the data and the hash tree on the
 * devices 'data_dev' and 'hash_dev' ("major:minor"), both starting at
 * offset 0
 *
 * @return 0 on success or -ENOSPC if 'size' is too small
 */
int ks_verity_table(char *buf,
                    size_t size,
                    const char *data_dev,
                    const char *hash_dev,
                    const struct ks_image *image,
                    enum ks_verity_mode mode);

/**
 * Create the dm-verity device /dev/mapper/<name> for 'image' on 'device'.
 * The file system and its hash tree are parts of the bpak and start at
 * 512 byte aligned offsets, while dm-verity reads the data and the tree from
 * the start of a device. Two linear devices, <name>-data and <name>-hash,
 * map the parts and the verity device is created on top of them.
 *
 * @param[in] device Block device of the image
 * @param[in] image The verified image header
 * @param[in] name Device name
 * @param[in] mode What dm-verity does on corruption
 * @param[out] path Path of the verity device node
 * @param[in] path_size Size of 'path'
 *
 * @return 0 on success or -errno
 */
int ks_verity_setup(const char *device,
                    const struct ks_image *image,
                    const char *name,
                    enum ks_verity_mode mode,
                    char *path,
                    size_t path_size);

#endif
