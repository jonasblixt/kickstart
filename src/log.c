/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "log.h"
#include <bpak/bpak.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static enum ks_log_level log_level = KS_LOG_INFO;
static int kmsg_fd = -1;

/* syslog priorities of the kernel log, see /dev/kmsg */
static const int kmsg_priority[] = {
    [KS_LOG_ERR] = 3,
    [KS_LOG_WARN] = 4,
    [KS_LOG_INFO] = 6,
    [KS_LOG_DBG] = 7,
};

static const char *level_prefix[] = {
    [KS_LOG_ERR] = "error: ",
    [KS_LOG_WARN] = "warning: ",
    [KS_LOG_INFO] = "",
    [KS_LOG_DBG] = "",
};

void ks_log_use_kmsg(void)
{
    if (kmsg_fd < 0)
        kmsg_fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
}

void ks_log_set_level(enum ks_log_level level)
{
    log_level = level;
}

static void vlog(enum ks_log_level level, const char *fmt, va_list ap)
{
    char msg[512];
    char line[600];
    int len;

    if (level > log_level)
        return;

    vsnprintf(msg, sizeof(msg), fmt, ap);

    /* Messages are single lines, the trailing newline is optional */
    len = strlen(msg);
    while (len > 0 && msg[len - 1] == '\n')
        msg[--len] = 0;

    if (kmsg_fd >= 0) {
        len = snprintf(line,
                       sizeof(line),
                       "<%d>kickstart: %s%s\n",
                       kmsg_priority[level],
                       level_prefix[level],
                       msg);
        if (write(kmsg_fd, line, len) == len && level > KS_LOG_WARN)
            return;
    }

    len = snprintf(line, sizeof(line), "kickstart: %s%s\n", level_prefix[level], msg);
    if (write(STDERR_FILENO, line, len) < 0)
        return;
}

void ks_log(enum ks_log_level level, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog(level, fmt, ap);
    va_end(ap);
}

/* Overrides the weak symbol of libbpak, its verbosity 0 is errors and 2 is
 * debug */
int bpak_printf(int verbosity, const char *fmt, ...)
{
    va_list ap;
    enum ks_log_level level = KS_LOG_DBG;

    if (verbosity == 0)
        level = KS_LOG_ERR;

    va_start(ap, fmt);
    vlog(level, fmt, ap);
    va_end(ap);

    return BPAK_OK;
}
