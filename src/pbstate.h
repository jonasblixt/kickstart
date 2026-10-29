/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_PBSTATE_H
#define KS_PBSTATE_H

#include "config.h"
#include <stdbool.h>
#include <stdint.h>

/* The boot state of punchboot, this is a copy of punchboot's
 * include/boot/pb_state_blob.h and must not change */

#define PB_STATE_MAGIC            0x026d4a65
#define PB_STATE_NO_OF_BOARD_REGS 4

struct pb_boot_state /* 512 bytes */
{
    uint32_t magic; /*!< PB boot state magic number */
    uint32_t enable; /*!< Boot partition enable bits */
    uint32_t verified; /*!< Boot partition verified bits */
    uint32_t remaining_boot_attempts; /*!< Rollback boot counter */
    uint32_t error; /*!< Rollback error bits */
    uint8_t rz[472]; /*!< Reserved, set to zero */
    uint32_t board_regs[PB_STATE_NO_OF_BOARD_REGS]; /*!< Board specific registers, in
                                                         reverse order */
    uint32_t crc; /*!< State checksum */
} __attribute__((packed));

_Static_assert(sizeof(struct pb_boot_state) == 512, "pb_boot_state is 512 bytes");

#define PB_STATE_A_ENABLED        (1 << 0)
#define PB_STATE_B_ENABLED        (1 << 1)
#define PB_STATE_A_VERIFIED       (1 << 0)
#define PB_STATE_B_VERIFIED       (1 << 1)

#define PB_STATE_ERROR_A_ROLLBACK (1 << 0)
#define PB_STATE_ERROR_B_ROLLBACK (1 << 1)

/**
 * Check the magic and the checksum of a state that is read from a pbstate
 * partition
 *
 * @return 0 if the state is valid or -EBADMSG
 */
int ks_pbstate_check(const struct pb_boot_state *state);

/**
 * Read the boot state from the primary device, or from the backup device
 * when the primary state is not valid. Read only, kickstart never writes the
 * state.
 *
 * @param[in] primary Primary state device
 * @param[in] backup Backup state device
 * @param[out] state The state
 * @param[out] from_backup Optional, true if the primary state was not valid
 *
 * @return 0 on success, -EBADMSG if none of the states are valid or -errno
 */
int ks_pbstate_read(const char *primary,
                    const char *backup,
                    struct pb_boot_state *state,
                    bool *from_backup);

bool ks_pbstate_enabled(const struct pb_boot_state *state, enum ks_system system);

bool ks_pbstate_verified(const struct pb_boot_state *state, enum ks_system system);

/** Board register 'index', the registers are stored in reverse order */
uint32_t ks_pbstate_board_reg(const struct pb_boot_state *state, unsigned int index);

#endif
