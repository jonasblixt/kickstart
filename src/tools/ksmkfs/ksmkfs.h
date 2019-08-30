
/**
 * Kickstart
 *
 * Copyright (C) 2019 Jonas Persson <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __KSMKFS_H___
#define __KSMKFS_H___

#include <stdint.h>
#include <stdbool.h>

uint32_t ksmkfs_prepare(uint32_t key_index,
                         uint32_t hash_kind,
                         uint32_t sign_kind,
                         const char *key_source,
                         const char *pkcs11_provider,
                         const char *pkcs11_key_id,
                         const char *input_fn);

void ksmkfs_cleanup(void);

uint32_t ksmkfs_out(const char *fn);

#endif
