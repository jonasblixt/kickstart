/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pbstate.h"
#include "crc32.h"
#include "log.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int ks_pbstate_check(const struct pb_boot_state *state)
{
    struct pb_boot_state tmp;

    if (state->magic != PB_STATE_MAGIC)
        return -EBADMSG;

    /* The checksum is computed with the crc field set to zero */
    memcpy(&tmp, state, sizeof(tmp));
    tmp.crc = 0;

    if (ks_crc32_update(0, &tmp, sizeof(tmp)) != state->crc)
        return -EBADMSG;

    return 0;
}

static int read_state(const char *device, struct pb_boot_state *state)
{
    int rc;
    int fd = open(device, O_RDONLY | O_CLOEXEC);

    if (fd < 0)
        return -errno;

    rc = ks_pread_all(fd, state, sizeof(*state), 0);
    close(fd);

    if (rc != 0)
        return rc;

    return ks_pbstate_check(state);
}

int ks_pbstate_read(const char *primary,
                    const char *backup,
                    struct pb_boot_state *state,
                    bool *from_backup)
{
    int rc = read_state(primary, state);

    if (from_backup)
        *from_backup = false;

    if (rc == 0)
        return 0;

    LOG_WARN("Primary pbstate %s is not valid (%i), trying the backup", primary, rc);

    rc = read_state(backup, state);

    if (rc != 0) {
        LOG_ERR("Backup pbstate %s is not valid (%i)", backup, rc);
        return rc;
    }

    if (from_backup)
        *from_backup = true;

    return 0;
}

bool ks_pbstate_enabled(const struct pb_boot_state *state, enum ks_system system)
{
    return state->enable & (system == KS_SYSTEM_A ? PB_STATE_A_ENABLED : PB_STATE_B_ENABLED);
}

bool ks_pbstate_verified(const struct pb_boot_state *state, enum ks_system system)
{
    return state->verified & (system == KS_SYSTEM_A ? PB_STATE_A_VERIFIED : PB_STATE_B_VERIFIED);
}

uint32_t ks_pbstate_board_reg(const struct pb_boot_state *state, unsigned int index)
{
    if (index >= PB_STATE_NO_OF_BOARD_REGS)
        return 0;
    return state->board_regs[PB_STATE_NO_OF_BOARD_REGS - index - 1];
}
