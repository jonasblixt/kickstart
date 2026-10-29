/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_TEST_COMMON_H
#define KS_TEST_COMMON_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
/* cmocka.h needs the headers above */
#include <cmocka.h>

#include "config.h"
#include "pbstate.h"

/* The images of mkfixtures.sh */
#define FIXTURE(name) KS_TEST_FIXTURES "/" name

/** A configuration with the test keystore and a temporary /chosen directory */
void test_config(struct ks_config *cfg);

/** Create a temporary directory, removed by test_cleanup() */
const char *test_tmpdir(void);

/** Remove the temporary directory */
void test_cleanup(void);

/** Write 'size' bytes to the file 'name' in the temporary directory */
const char *test_write_file(const char *name, const void *data, size_t size);

/** Write a device tree string property to /chosen */
void test_chosen_string(const char *name, const char *value);

/** Write a device tree property of 'count' cells to /chosen */
void test_chosen_cells(const char *name, const uint32_t *cells, size_t count);

/** A valid pbstate with a checksum */
void test_pbstate(struct pb_boot_state *state, uint32_t enable, uint32_t verified);

#endif
