/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "crc32.h"

uint32_t ks_crc32_update(uint32_t crc, const void *buf, size_t size)
{
    const uint8_t *p = buf;

    while (size--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1));
    }

    return crc;
}

uint32_t ks_crc32(const void *buf, size_t size)
{
    return ~ks_crc32_update(~0u, buf, size);
}
