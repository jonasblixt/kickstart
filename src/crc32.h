/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_CRC32_H
#define KS_CRC32_H

#include <stddef.h>
#include <stdint.h>

/**
 * CRC32 (polynomial 0xedb88320) without the initial and final inversion.
 * This is the 'crc32()' of punchboot, the pbstate checksum is
 * ks_crc32_update(0, ...).
 */
uint32_t ks_crc32_update(uint32_t crc, const void *buf, size_t size);

/** The common CRC32 of zlib and the GPT, with the inversions */
uint32_t ks_crc32(const void *buf, size_t size);

#endif
