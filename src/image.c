/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "image.h"
#include "log.h"
#include "util.h"
#include <bpak/crypto.h>
#include <bpak/id.h>
#include <bpak/keystore.h>
#include <bpak/verify.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static ssize_t read_fd(off_t offset, uint8_t *buf, size_t length, void *user)
{
    int fd = *(int *)user;
    int rc = ks_pread_all(fd, buf, length, offset);

    return rc == 0 ? (ssize_t)length : -BPAK_READ_ERROR;
}

static int check_key_policy(const struct ks_config *cfg,
                            const struct ks_chosen *chosen,
                            uint32_t key_id)
{
    if (!chosen->has_available_keys) {
        if (cfg->require_pb_keys) {
            LOG_ERR("punchboot did not set /chosen/pb,slc-available-keys");
            return -BPAK_KEY_NOT_FOUND;
        }
        return 0;
    }

    for (size_t i = 0; i < chosen->no_of_available_keys; i++) {
        if (chosen->available_keys[i] == key_id)
            return 0;
    }

    if (cfg->require_pb_keys) {
        LOG_ERR("Key %08x is revoked or not known by punchboot", key_id);
        return -BPAK_KEY_NOT_FOUND;
    }

    LOG_WARN("Key %08x is not in pb,slc-available-keys", key_id);
    return 0;
}

static int verify_signature(const struct ks_config *cfg,
                            const struct ks_chosen *chosen,
                            struct bpak_header *hdr)
{
    int rc;
    uint8_t hash[BPAK_HASH_MAX_LENGTH];
    size_t hash_size = sizeof(hash);
    struct bpak_key *key = NULL;
    bool verified = false;

    if (cfg->keystore->id != hdr->keystore_id) {
        LOG_ERR("Image keystore %08x, kickstart has keystore %08x",
                hdr->keystore_id,
                cfg->keystore->id);
        return -BPAK_KEYSTORE_ID_MISMATCH;
    }

    rc = check_key_policy(cfg, chosen, hdr->key_id);

    if (rc != 0)
        return rc;

    rc = bpak_keystore_get((struct bpak_keystore *)cfg->keystore, hdr->key_id, &key);

    if (rc != BPAK_OK) {
        LOG_ERR("Key %08x is not in the keystore (%i)", hdr->key_id, rc);
        return rc;
    }

    if (hdr->signature_sz > sizeof(hdr->signature))
        return -BPAK_SIZE_ERROR;

    rc = bpak_verify_compute_header_hash(hdr, hash, &hash_size);

    if (rc != BPAK_OK)
        return rc;

    rc = bpak_crypto_verify(
        hdr->signature, hdr->signature_sz, hash, hash_size, hdr->hash_kind, key, &verified);

    if (rc != BPAK_OK)
        return rc;

    if (!verified)
        return -BPAK_VERIFY_FAIL;

    LOG_INFO("Signature OK, key %08x", hdr->key_id);
    return 0;
}

static int verify_payload_hash(int fd, struct bpak_header *hdr)
{
    int rc;
    uint8_t hash[BPAK_HASH_MAX_LENGTH];
    size_t hash_size = sizeof(hash);

    /* Only the parts that are not excluded from the payload hash are read,
     * the file system and its tree should be excluded ('dont-hash') and are
     * verified by dm-verity instead */
    rc = bpak_verify_compute_payload_hash(hdr, read_fd, sizeof(*hdr), &fd, hash, &hash_size);

    if (rc != BPAK_OK)
        return rc;

    if (memcmp(hash, hdr->payload_hash, hash_size) != 0)
        return -BPAK_BAD_PAYLOAD_HASH;

    return 0;
}

static int
get_meta_hash(struct bpak_header *hdr, bpak_id_t id, bpak_id_t part_id, bpak_merkle_hash_t out)
{
    struct bpak_meta_header *meta;
    int rc = bpak_get_meta(hdr, id, part_id, &meta);

    if (rc != BPAK_OK)
        return -BPAK_MISSING_META_DATA;

    if (meta->size != sizeof(bpak_merkle_hash_t))
        return -BPAK_SIZE_ERROR;

    memcpy(out, bpak_get_meta_ptr(hdr, meta, uint8_t), sizeof(bpak_merkle_hash_t));
    return 0;
}

static int locate_parts(const struct ks_config *cfg, struct ks_image *image)
{
    int rc;
    struct bpak_header *hdr = &image->header;
    struct bpak_part_header *data_part, *tree_part;
    struct bpak_meta_header *meta;
    char tree_name[sizeof("-hash-tree") + 64];
    ssize_t expected_tree_size;

    snprintf(tree_name, sizeof(tree_name), "%s-hash-tree", cfg->root_part);

    rc = bpak_get_part(hdr, bpak_id(cfg->root_part), &data_part);

    if (rc != BPAK_OK) {
        LOG_ERR("Image has no part '%s'", cfg->root_part);
        return rc;
    }

    rc = bpak_get_part(hdr, bpak_id(tree_name), &tree_part);

    if (rc != BPAK_OK) {
        LOG_ERR("Image has no part '%s'", tree_name);
        return rc;
    }

    if ((data_part->flags | tree_part->flags) & BPAK_FLAG_TRANSPORT) {
        LOG_ERR("Image is transport encoded");
        return -BPAK_NOT_SUPPORTED;
    }

    if (!(data_part->flags & BPAK_FLAG_EXCLUDE_FROM_HASH))
        LOG_WARN("'%s' is included in the payload hash, it is read twice", cfg->root_part);

    image->data_offset = bpak_part_offset(hdr, data_part);
    image->data_size = data_part->size;
    image->tree_offset = bpak_part_offset(hdr, tree_part);
    image->tree_size = tree_part->size;

    expected_tree_size = bpak_merkle_compute_size(image->data_size);

    if (expected_tree_size < 0 || (uint64_t)expected_tree_size != image->tree_size) {
        LOG_ERR("Hash tree size %llu does not match the file system size %llu",
                (unsigned long long)image->tree_size,
                (unsigned long long)image->data_size);
        return -BPAK_SIZE_ERROR;
    }

    rc = get_meta_hash(hdr, BPAK_ID_MERKLE_ROOT_HASH, data_part->id, image->root_hash);

    if (rc != 0) {
        LOG_ERR("Could not read merkle-root-hash of '%s' (%i)", cfg->root_part, rc);
        return rc;
    }

    rc = get_meta_hash(hdr, BPAK_ID_MERKLE_SALT, data_part->id, image->salt);

    if (rc != 0) {
        LOG_ERR("Could not read merkle-salt of '%s' (%i)", cfg->root_part, rc);
        return rc;
    }

    if (bpak_get_meta(hdr, BPAK_ID_BPAK_PACKAGE, 0, &meta) == BPAK_OK &&
        meta->size == sizeof(image->package_uuid)) {
        memcpy(image->package_uuid, bpak_get_meta_ptr(hdr, meta, uint8_t), meta->size);
        image->has_package_uuid = true;
    }

    return 0;
}

int ks_image_load(const char *device,
                  const struct ks_config *cfg,
                  const struct ks_chosen *chosen,
                  struct ks_image *image)
{
    int rc;
    uint64_t device_size;
    int fd = open(device, O_RDONLY | O_CLOEXEC);

    memset(image, 0, sizeof(*image));

    if (fd < 0) {
        rc = -errno;
        LOG_ERR("Could not open %s (%i)", device, rc);
        return rc;
    }

    rc = ks_pread_all(fd, &image->header, sizeof(image->header), 0);

    if (rc != 0) {
        LOG_ERR("Could not read the bpak header of %s (%i)", device, rc);
        goto err_close_out;
    }

    /* The header is checked before it is used, the sizes are not trusted
     * until the signature is verified */
    rc = bpak_valid_header(&image->header);

    if (rc != BPAK_OK) {
        LOG_ERR("%s: invalid bpak header (%s)", device, bpak_error_string(rc));
        goto err_close_out;
    }

    rc = verify_signature(cfg, chosen, &image->header);

    if (rc != 0) {
        LOG_ERR("%s: signature verification failed (%s)", device, bpak_error_string(rc));
        goto err_close_out;
    }

    rc = verify_payload_hash(fd, &image->header);

    if (rc != 0) {
        LOG_ERR("%s: payload verification failed (%s)", device, bpak_error_string(rc));
        goto err_close_out;
    }

    rc = locate_parts(cfg, image);

    if (rc != 0)
        goto err_close_out;

    rc = ks_fd_size(fd, &device_size);

    if (rc != 0) {
        LOG_ERR("Could not get the size of %s (%i)", device, rc);
        goto err_close_out;
    }

    if (image->tree_offset + image->tree_size > device_size ||
        image->data_offset + image->data_size > device_size) {
        LOG_ERR("%s is smaller than the image", device);
        rc = -BPAK_SIZE_ERROR;
        goto err_close_out;
    }

err_close_out:
    close(fd);
    return rc;
}
