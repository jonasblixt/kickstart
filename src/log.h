/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef KS_LOG_H
#define KS_LOG_H

enum ks_log_level {
    KS_LOG_ERR,
    KS_LOG_WARN,
    KS_LOG_INFO,
    KS_LOG_DBG,
};

/**
 * Send log messages to the kernel log (/dev/kmsg) instead of stderr. Errors
 * and warnings are written to stderr as well, the kernel rate limits the
 * messages from user space unless the kernel is booted with
 * printk.devkmsg=on.
 */
void ks_log_use_kmsg(void);

void ks_log_set_level(enum ks_log_level level);

void ks_log(enum ks_log_level level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define LOG_ERR(...)  ks_log(KS_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) ks_log(KS_LOG_WARN, __VA_ARGS__)
#define LOG_INFO(...) ks_log(KS_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...)  ks_log(KS_LOG_DBG, __VA_ARGS__)

#endif
