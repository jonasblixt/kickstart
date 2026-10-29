/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_IMAGE_H
#define KS_IMAGE_H

#include "chosen.h"
#include "config.h"
#include <bpak/bpak.h>
#include <bpak/merkle.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * A verified root image: a bpak with a file system part and its merkle hash
 * tree, created with 'bpak add --encoder merkle'. The bpak header is signed,
 * the root hash and the salt of the tree are meta data in the header. The
 * file system is verified block by block by dm-verity when it is read.
 */
struct ks_image {
    struct bpak_header header;
    bool has_package_uuid;
    uint8_t package_uuid[16];
    uint64_t data_offset; /*!< Offset in bytes of the file system on the device */
    uint64_t data_size; /*!< Size in bytes, a multiple of BPAK_MERKLE_BLOCK_SZ */
    uint64_t tree_offset; /*!< Offset in bytes of the hash tree */
    uint64_t tree_size;
    bpak_merkle_hash_t root_hash;
    bpak_merkle_hash_t salt;
};

/**
 * Read and verify the root image header on 'device': the signature with a
 * key from the keystore, the payload hash of the parts that are not excluded
 * from it, and the layout of the file system part and its hash tree.
 *
 * @param[in] device Block device or file
 * @param[in] cfg Configuration, the part name, the keystore and the key policy
 * @param[in] chosen The keys that punchboot accepts
 * @param[out] image The verified image
 *
 * @return 0 on success or a negative bpak error code or -errno
 */
int ks_image_load(const char *device,
                  const struct ks_config *cfg,
                  const struct ks_chosen *chosen,
                  struct ks_image *image);

#endif
