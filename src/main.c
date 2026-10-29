/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "boot.h"
#include "config.h"
#include "dm.h"
#include "log.h"
#include "rootfs.h"
#include "util.h"
#include <kickstart/build_config.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <unistd.h>

#define KS_VERITY_NAME "kickstart-root"

static void __attribute__((noreturn)) panic(void)
{
    LOG_ERR("Boot failed, restarting in 1 s");
    sync();
    ks_sleep_ms(1000);
    reboot(RB_AUTOBOOT);

    /* PID 1 must never exit */
    for (;;)
        pause();
}

static int dry_run(const struct ks_config *cfg, const struct ks_boot *boot)
{
    char table[512];
    int rc;

    /* The linear devices of the data and the hash tree are created on boot */
    rc = ks_verity_table(table,
                         sizeof(table),
                         "<" KS_VERITY_NAME "-data>",
                         "<" KS_VERITY_NAME "-hash>",
                         &boot->image,
                         cfg->verity_mode);

    if (rc != 0)
        return rc;

    printf("system:  %s\n", ks_system_name(boot->system));
    printf("root:    %s\n", boot->root_device);
    printf("verity:  0 %llu verity %s\n", (unsigned long long)(boot->image.data_size / 512), table);
    printf("mount:   " KS_NEWROOT " %s ro %s\n", cfg->root_fstype, cfg->root_options);
    printf("init:    %s\n", cfg->init);
    return 0;
}

int main(int argc, char **argv)
{
    struct ks_config cfg;
    struct ks_boot boot;
    char root_path[64];
    int rc;
    bool pid1 = getpid() == 1;

    (void)argc;

    ks_config_init(&cfg);
    ks_log_set_level(cfg.log_level);

    if (pid1) {
        if (ks_early_mounts() != 0)
            panic();
        ks_log_use_kmsg();
    }

    LOG_INFO("Kickstart " KS_VERSION "%s", pid1 ? "" : " (dry run, not PID 1)");

    rc = ks_boot_prepare(&cfg, &boot);

    if (!pid1) {
        if (rc == 0)
            rc = dry_run(&cfg, &boot);
        return rc == 0 ? 0 : 1;
    }

    if (rc != 0)
        panic();

    rc = ks_verity_setup(boot.root_device,
                         &boot.image,
                         KS_VERITY_NAME,
                         cfg.verity_mode,
                         root_path,
                         sizeof(root_path));

    if (rc != 0)
        panic();

    if (ks_mount_root(root_path, cfg.root_fstype, cfg.root_options) != 0)
        panic();

    ks_switch_root(cfg.init, argv);
    panic();
}
