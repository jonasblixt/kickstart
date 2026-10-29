/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "common.h"
#include "image.h"
#include <bpak/id.h>
#include <errno.h>
#include <string.h>

static struct ks_config cfg;
static struct ks_chosen chosen;

static int setup(void **state)
{
    (void)state;
    test_config(&cfg);

    /* punchboot accepts the test key */
    memset(&chosen, 0, sizeof(chosen));
    chosen.has_available_keys = true;
    chosen.no_of_available_keys = 2;
    chosen.available_keys[0] = 0xa90f9680;
    chosen.available_keys[1] = bpak_id("test-key");
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    test_cleanup();
    return 0;
}

static void test_valid(void **state)
{
    (void)state;
    struct ks_image image;
    static const uint8_t package_uuid[16] = { 0x8c, 0x4a, 0x8b, 0x5e, 0x2d, 0x46, 0x4b, 0xb4,
                                              0xa9, 0xb7, 0x58, 0xa0, 0xb0, 0xb9, 0xb3, 0xc4 };
    static const uint8_t zero[32];

    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image), 0);

    /* The file system follows the header, the tree follows the file system */
    assert_int_equal(image.data_offset, 4096);
    assert_int_equal(image.data_size % 4096, 0);
    assert_int_equal(image.tree_offset, image.data_offset + image.data_size);
    assert_int_equal(image.tree_size, bpak_merkle_compute_size(image.data_size));
    assert_true(image.has_package_uuid);
    assert_memory_equal(image.package_uuid, package_uuid, 16);
    assert_memory_not_equal(image.root_hash, zero, 32);
    assert_memory_not_equal(image.salt, zero, 32);
}

static void test_unaligned(void **state)
{
    (void)state;
    struct ks_image image;

    /* A 1000 byte part before the file system, padded to 1024 */
    assert_int_equal(ks_image_load(FIXTURE("rootfs-unaligned.bpak"), &cfg, &chosen, &image), 0);
    assert_int_equal(image.data_offset, 4096 + 1024);
    assert_int_equal(image.data_offset % 512, 0);
    assert_int_equal(image.tree_offset, image.data_offset + image.data_size);
}

static void test_hashed(void **state)
{
    (void)state;
    struct ks_image image;

    /* The file system is included in the payload hash */
    assert_int_equal(ks_image_load(FIXTURE("rootfs-hashed.bpak"), &cfg, &chosen, &image), 0);
}

static void test_bad_payload(void **state)
{
    (void)state;
    struct ks_image image;

    assert_int_equal(ks_image_load(FIXTURE("rootfs-bad-payload.bpak"), &cfg, &chosen, &image),
                     -BPAK_BAD_PAYLOAD_HASH);
}

static void test_bad_header(void **state)
{
    (void)state;
    struct ks_image image;

    assert_int_equal(ks_image_load(FIXTURE("rootfs-bad-header.bpak"), &cfg, &chosen, &image),
                     -BPAK_VERIFY_FAIL);
}

static void test_other_key(void **state)
{
    (void)state;
    struct ks_image image;

    /* Signed with a key that is not the key in the keystore */
    assert_int_equal(ks_image_load(FIXTURE("rootfs-other-key.bpak"), &cfg, &chosen, &image),
                     -BPAK_VERIFY_FAIL);
}

static void test_revoked_key(void **state)
{
    (void)state;
    struct ks_image image;

    chosen.no_of_available_keys = 1;
    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image),
                     -BPAK_KEY_NOT_FOUND);

    /* Allowed when the key policy is off */
    cfg.require_pb_keys = false;
    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image), 0);
}

static void test_no_available_keys(void **state)
{
    (void)state;
    struct ks_image image;

    chosen.has_available_keys = false;
    chosen.no_of_available_keys = 0;
    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image),
                     -BPAK_KEY_NOT_FOUND);

    cfg.require_pb_keys = false;
    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image), 0);
}

static void test_wrong_keystore(void **state)
{
    (void)state;
    struct ks_image image;

    struct {
        struct bpak_keystore ks;
        struct bpak_key *keys[1];
    } other;

    memcpy(&other.ks, cfg.keystore, sizeof(other.ks));
    other.keys[0] = cfg.keystore->keys[0];
    other.ks.id = bpak_id("another-keystore");
    cfg.keystore = &other.ks;

    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image),
                     -BPAK_KEYSTORE_ID_MISMATCH);
}

static void test_missing_part(void **state)
{
    (void)state;
    struct ks_image image;

    cfg.root_part = "nonexistent";
    assert_int_equal(ks_image_load(FIXTURE("rootfs.bpak"), &cfg, &chosen, &image), -BPAK_NOT_FOUND);
}

static void test_truncated(void **state)
{
    (void)state;
    struct ks_image image;

    assert_int_equal(ks_image_load(FIXTURE("rootfs-truncated.bpak"), &cfg, &chosen, &image),
                     -BPAK_SIZE_ERROR);
}

static void test_not_a_bpak(void **state)
{
    (void)state;
    struct ks_image image;

    assert_int_equal(ks_image_load(FIXTURE("rootfs.squashfs"), &cfg, &chosen, &image),
                     -BPAK_BAD_MAGIC);
    assert_int_equal(ks_image_load(FIXTURE("nonexistent"), &cfg, &chosen, &image), -ENOENT);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_unaligned, setup, teardown),
        cmocka_unit_test_setup_teardown(test_hashed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_payload, setup, teardown),
        cmocka_unit_test_setup_teardown(test_bad_header, setup, teardown),
        cmocka_unit_test_setup_teardown(test_other_key, setup, teardown),
        cmocka_unit_test_setup_teardown(test_revoked_key, setup, teardown),
        cmocka_unit_test_setup_teardown(test_no_available_keys, setup, teardown),
        cmocka_unit_test_setup_teardown(test_wrong_keystore, setup, teardown),
        cmocka_unit_test_setup_teardown(test_missing_part, setup, teardown),
        cmocka_unit_test_setup_teardown(test_truncated, setup, teardown),
        cmocka_unit_test_setup_teardown(test_not_a_bpak, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
