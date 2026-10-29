/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

ssize_t ks_read_file(const char *path, void *buf, size_t size)
{
    size_t total = 0;
    int fd = open(path, O_RDONLY | O_CLOEXEC);

    if (fd < 0)
        return -errno;

    while (total < size) {
        ssize_t n = read(fd, (uint8_t *)buf + total, size - total);

        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0) {
            int err = -errno;
            close(fd);
            return err;
        }
        if (n == 0)
            break;
        total += n;
    }

    close(fd);
    return total;
}

int ks_pread_all(int fd, void *buf, size_t size, off_t offset)
{
    size_t total = 0;

    while (total < size) {
        ssize_t n = pread(fd, (uint8_t *)buf + total, size - total, offset + total);

        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0)
            return -errno;
        if (n == 0)
            return -EIO;
        total += n;
    }

    return 0;
}

int ks_fd_size(int fd, uint64_t *size)
{
    struct stat st;

    if (fstat(fd, &st) != 0)
        return -errno;

    if (S_ISBLK(st.st_mode)) {
        if (ioctl(fd, BLKGETSIZE64, size) != 0)
            return -errno;
        return 0;
    }

    if (S_ISREG(st.st_mode)) {
        *size = st.st_size;
        return 0;
    }

    return -ENOTBLK;
}

void ks_hex(const uint8_t *data, size_t len, char *out)
{
    static const char digits[] = "0123456789abcdef";

    for (size_t i = 0; i < len; i++) {
        out[2 * i] = digits[data[i] >> 4];
        out[2 * i + 1] = digits[data[i] & 0xf];
    }

    out[2 * len] = 0;
}

int ks_mkdir_p(const char *path, mode_t mode)
{
    char tmp[256];
    size_t len = strlen(path);

    if (len == 0 || len >= sizeof(tmp))
        return -EINVAL;

    memcpy(tmp, path, len + 1);

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/')
            continue;
        *p = 0;
        if (mkdir(tmp, mode) != 0 && errno != EEXIST)
            return -errno;
        *p = '/';
    }

    if (mkdir(tmp, mode) != 0 && errno != EEXIST)
        return -errno;

    return 0;
}

void ks_sleep_ms(unsigned int ms)
{
    struct timespec ts = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L,
    };

    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

uint64_t ks_now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
