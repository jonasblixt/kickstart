/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "dm.h"
#include "log.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/dm-ioctl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#define DM_CONTROL     "/dev/mapper/control"
#define DM_BUFFER_SIZE 4096

/* An ioctl buffer, struct dm_ioctl followed by the target spec and its
 * parameters */
union dm_buffer {
    struct dm_ioctl io;
    uint8_t data[DM_BUFFER_SIZE];
};

static void dm_init(union dm_buffer *buf, const char *name)
{
    memset(buf, 0, sizeof(*buf));
    buf->io.version[0] = DM_VERSION_MAJOR;
    buf->io.version[1] = 0;
    buf->io.version[2] = 0;
    buf->io.data_size = sizeof(*buf);
    buf->io.data_start = sizeof(struct dm_ioctl);
    snprintf(buf->io.name, sizeof(buf->io.name), "%s", name);
}

static int dm_open_control(void)
{
    int fd = open(DM_CONTROL, O_RDWR | O_CLOEXEC);

    if (fd < 0) {
        int err = -errno;
        LOG_ERR("Could not open " DM_CONTROL " (%i), is CONFIG_BLK_DEV_DM enabled?", err);
        return err;
    }

    return fd;
}

static int dm_ioctl(int fd, unsigned long cmd, union dm_buffer *buf, const char *what)
{
    if (ioctl(fd, cmd, buf) != 0) {
        int err = -errno;
        LOG_ERR("%s %s failed (%i)", what, buf->io.name, err);
        return err;
    }

    return 0;
}

static int dm_mknod(const char *name, dev_t dev)
{
    char path[DM_NAME_LEN + 16];

    snprintf(path, sizeof(path), "/dev/mapper/%s", name);

    /* devtmpfs only creates /dev/dm-<n> */
    if (mknod(path, S_IFBLK | 0600, dev) != 0 && errno != EEXIST) {
        int err = -errno;
        LOG_ERR("Could not create %s (%i)", path, err);
        return err;
    }

    return 0;
}

int ks_dm_create(const char *name,
                 const char *target,
                 uint64_t sectors,
                 const char *params,
                 dev_t *dev)
{
    int rc;
    union dm_buffer buf;
    struct dm_target_spec *spec;
    size_t params_offset = sizeof(struct dm_ioctl) + sizeof(struct dm_target_spec);
    int fd;

    if (strlen(name) >= DM_NAME_LEN || strlen(target) >= DM_MAX_TYPE_NAME ||
        params_offset + strlen(params) + 1 > sizeof(buf))
        return -EINVAL;

    fd = dm_open_control();

    if (fd < 0)
        return fd;

    dm_init(&buf, name);
    rc = dm_ioctl(fd, DM_DEV_CREATE, &buf, "DM_DEV_CREATE");

    if (rc != 0)
        goto err_close_out;

    dm_init(&buf, name);
    buf.io.target_count = 1;
    buf.io.flags = DM_READONLY_FLAG;
    spec = (struct dm_target_spec *)&buf.data[sizeof(struct dm_ioctl)];
    spec->sector_start = 0;
    spec->length = sectors;
    snprintf(spec->target_type, sizeof(spec->target_type), "%s", target);
    strcpy((char *)&buf.data[params_offset], params);

    LOG_DBG("%s: 0 %llu %s %s", name, (unsigned long long)sectors, target, params);

    rc = dm_ioctl(fd, DM_TABLE_LOAD, &buf, "DM_TABLE_LOAD");

    if (rc != 0)
        goto err_remove_out;

    /* Resuming a device activates the loaded table */
    dm_init(&buf, name);
    rc = dm_ioctl(fd, DM_DEV_SUSPEND, &buf, "DM_DEV_SUSPEND");

    if (rc != 0)
        goto err_remove_out;

    *dev = buf.io.dev;
    close(fd);
    return 0;

err_remove_out:
    dm_init(&buf, name);
    (void)ioctl(fd, DM_DEV_REMOVE, &buf);
err_close_out:
    close(fd);
    return rc;
}

int ks_dm_remove(const char *name)
{
    int rc;
    union dm_buffer buf;
    char path[DM_NAME_LEN + 16];
    int fd = dm_open_control();

    if (fd < 0)
        return fd;

    dm_init(&buf, name);
    rc = dm_ioctl(fd, DM_DEV_REMOVE, &buf, "DM_DEV_REMOVE");
    close(fd);

    snprintf(path, sizeof(path), "/dev/mapper/%s", name);
    (void)unlink(path);

    return rc;
}

static const char *verity_mode_arg(enum ks_verity_mode mode)
{
    switch (mode) {
    case KS_VERITY_RESTART:
        return " 1 restart_on_corruption";
    case KS_VERITY_PANIC:
        return " 1 panic_on_corruption";
    default:
        return "";
    }
}

int ks_verity_table(char *buf,
                    size_t size,
                    const char *data_dev,
                    const char *hash_dev,
                    const struct ks_image *image,
                    enum ks_verity_mode mode)
{
    char root_hash[2 * sizeof(image->root_hash) + 1];
    char salt[2 * sizeof(image->salt) + 1];
    int len;

    ks_hex(image->root_hash, sizeof(image->root_hash), root_hash);
    ks_hex(image->salt, sizeof(image->salt), salt);

    /* <version> <data dev> <hash dev> <data block size> <hash block size>
     * <number of data blocks> <hash start block> <algorithm> <root hash>
     * <salt> [<number of optional args> <args>...]
     *
     * Version 1 hashes the salt before each block and the tree has the top
     * level first, the format of the bpak merkle tree */
    len = snprintf(buf,
                   size,
                   "1 %s %s %u %u %llu 0 sha256 %s %s%s",
                   data_dev,
                   hash_dev,
                   BPAK_MERKLE_BLOCK_SZ,
                   BPAK_MERKLE_BLOCK_SZ,
                   (unsigned long long)(image->data_size / BPAK_MERKLE_BLOCK_SZ),
                   root_hash,
                   salt,
                   verity_mode_arg(mode));

    if (len < 0 || (size_t)len >= size)
        return -ENOSPC;

    return 0;
}

static int linear_window(const char *name, dev_t dev, uint64_t offset, uint64_t size, dev_t *out)
{
    char params[64];

    if (offset % 512 != 0 || size % 512 != 0) {
        LOG_ERR("%s is not 512 byte aligned", name);
        return -EINVAL;
    }

    snprintf(params,
             sizeof(params),
             "%u:%u %llu",
             major(dev),
             minor(dev),
             (unsigned long long)(offset / 512));

    return ks_dm_create(name, "linear", size / 512, params, out);
}

int ks_verity_setup(const char *device,
                    const struct ks_image *image,
                    const char *name,
                    enum ks_verity_mode mode,
                    char *path,
                    size_t path_size)
{
    int rc;
    struct stat st;
    char data_name[DM_NAME_LEN];
    char hash_name[DM_NAME_LEN];
    char table[512];
    char data_str[32], hash_str[32];
    dev_t data_dev, hash_dev, verity_dev;

    if (stat(device, &st) != 0) {
        rc = -errno;
        LOG_ERR("Could not stat %s (%i)", device, rc);
        return rc;
    }

    if (!S_ISBLK(st.st_mode)) {
        LOG_ERR("%s is not a block device", device);
        return -ENOTBLK;
    }

    rc = ks_mkdir_p("/dev/mapper", 0755);

    if (rc != 0)
        return rc;

    snprintf(data_name, sizeof(data_name), "%s-data", name);
    snprintf(hash_name, sizeof(hash_name), "%s-hash", name);

    rc = linear_window(data_name, st.st_rdev, image->data_offset, image->data_size, &data_dev);

    if (rc != 0)
        return rc;

    rc = linear_window(hash_name, st.st_rdev, image->tree_offset, image->tree_size, &hash_dev);

    if (rc != 0)
        goto err_remove_data_out;

    snprintf(data_str, sizeof(data_str), "%u:%u", major(data_dev), minor(data_dev));
    snprintf(hash_str, sizeof(hash_str), "%u:%u", major(hash_dev), minor(hash_dev));

    rc = ks_verity_table(table, sizeof(table), data_str, hash_str, image, mode);

    if (rc != 0)
        goto err_remove_hash_out;

    rc = ks_dm_create(name, "verity", image->data_size / 512, table, &verity_dev);

    if (rc != 0)
        goto err_remove_hash_out;

    rc = dm_mknod(name, verity_dev);

    if (rc != 0)
        goto err_remove_verity_out;

    snprintf(path, path_size, "/dev/mapper/%s", name);
    return 0;

err_remove_verity_out:
    ks_dm_remove(name);
err_remove_hash_out:
    ks_dm_remove(hash_name);
err_remove_data_out:
    ks_dm_remove(data_name);
    return rc;
}
