/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "chosen.h"
#include "log.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* Device tree properties are files with the raw value, strings are NUL
 * terminated and integers are big endian cells */

static ssize_t read_prop(const char *dir, const char *name, void *buf, size_t size)
{
    char path[512];

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    return ks_read_file(path, buf, size);
}

static int read_string(const char *dir, const char *name, char *out, size_t size, bool *found)
{
    char buf[256];
    ssize_t len = read_prop(dir, name, buf, sizeof(buf));

    if (len == -ENOENT)
        return 0;
    if (len < 0)
        return len;

    /* The value includes the NUL terminator */
    if (len == 0 || buf[len - 1] != 0 || strlen(buf) >= size) {
        LOG_ERR("Invalid string property %s", name);
        return -EINVAL;
    }

    strcpy(out, buf);
    *found = true;
    return 0;
}

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int
read_cells(const char *dir, const char *name, uint32_t *out, size_t max, size_t *count, bool *found)
{
    uint8_t buf[KS_CHOSEN_MAX_KEYS * 4 + 4];
    ssize_t len = read_prop(dir, name, buf, sizeof(buf));

    if (len == -ENOENT)
        return 0;
    if (len < 0)
        return len;

    if (len % 4 != 0 || (size_t)len / 4 > max) {
        LOG_ERR("Invalid cell property %s", name);
        return -EINVAL;
    }

    for (ssize_t i = 0; i < len / 4; i++)
        out[i] = be32(&buf[i * 4]);

    *count = len / 4;
    *found = true;
    return 0;
}

static int read_u32(const char *dir, const char *name, uint32_t *out, bool *found)
{
    size_t count = 0;
    int rc = read_cells(dir, name, out, 1, &count, found);

    if (rc == 0 && *found && count != 1) {
        LOG_ERR("Invalid u32 property %s", name);
        return -EINVAL;
    }

    return rc;
}

int ks_chosen_read(const char *path, struct ks_chosen *chosen)
{
    struct stat st;
    int rc;

    memset(chosen, 0, sizeof(*chosen));

    if (stat(path, &st) != 0)
        return -errno;
    if (!S_ISDIR(st.st_mode))
        return -ENOTDIR;

    rc = read_string(path,
                     "pb,active-system",
                     chosen->active_system,
                     sizeof(chosen->active_system),
                     &chosen->has_active_system);
    if (rc != 0)
        return rc;

    rc = read_string(path,
                     "pb,device-uuid",
                     chosen->device_uuid,
                     sizeof(chosen->device_uuid),
                     &chosen->has_device_uuid);
    if (rc != 0)
        return rc;

    rc = read_u32(path, "pb,slc", &chosen->slc, &chosen->has_slc);
    if (rc != 0)
        return rc;

    rc = read_u32(path, "pb,slc-active-key", &chosen->active_key, &chosen->has_active_key);
    if (rc != 0)
        return rc;

    return read_cells(path,
                      "pb,slc-available-keys",
                      chosen->available_keys,
                      KS_CHOSEN_MAX_KEYS,
                      &chosen->no_of_available_keys,
                      &chosen->has_available_keys);
}

const char *ks_slc_name(uint32_t slc)
{
    switch (slc) {
    case KS_SLC_NOT_CONFIGURED:
        return "not-configured";
    case KS_SLC_CONFIGURATION:
        return "configuration";
    case KS_SLC_CONFIGURATION_LOCKED:
        return "configuration-locked";
    case KS_SLC_EOL:
        return "eol";
    default:
        return "invalid";
    }
}
