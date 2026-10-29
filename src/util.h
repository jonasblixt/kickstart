/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_UTIL_H
#define KS_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define KS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * Read up to 'size' bytes of the file 'path'
 *
 * @return The number of bytes read or -errno
 */
ssize_t ks_read_file(const char *path, void *buf, size_t size);

/**
 * Read exactly 'size' bytes at 'offset'
 *
 * @return 0 on success, -EIO on a short read or -errno
 */
int ks_pread_all(int fd, void *buf, size_t size, off_t offset);

/** Size in bytes of a block device or regular file, or -errno */
int ks_fd_size(int fd, uint64_t *size);

/** Write 'len' bytes as lower case hex, 'out' needs 2 * len + 1 bytes */
void ks_hex(const uint8_t *data, size_t len, char *out);

/** Like 'mkdir -p' */
int ks_mkdir_p(const char *path, mode_t mode);

/** Sleep for 'ms' milliseconds */
void ks_sleep_ms(unsigned int ms);

/** Monotonic time in milliseconds */
uint64_t ks_now_ms(void);

#endif
