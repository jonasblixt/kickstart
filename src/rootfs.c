/**
 * Kickstart
 *
 * Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "rootfs.h"
#include "log.h"
#include "util.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

static const struct early_mount {
    const char *target;
    const char *fstype;
    unsigned long flags;
    const char *options;
} early_mounts[] = {
    { "/dev", "devtmpfs", MS_NOSUID, "mode=0755" },
    { "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL },
    { "/sys", "sysfs", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL },
    { "/run", "tmpfs", MS_NOSUID | MS_NODEV, "mode=0755" },
};

static void open_console(void)
{
    int fd;

    /* The kernel opens /dev/console for init when the initramfs has it,
     * otherwise stdin, stdout and stderr are closed */
    if (fcntl(STDERR_FILENO, F_GETFD) != -1)
        return;

    fd = open("/dev/console", O_RDWR | O_NOCTTY);

    if (fd < 0)
        return;

    for (int i = 0; i < 3; i++) {
        if (fd != i)
            dup2(fd, i);
    }

    if (fd > STDERR_FILENO)
        close(fd);
}

int ks_early_mounts(void)
{
    for (size_t i = 0; i < KS_ARRAY_SIZE(early_mounts); i++) {
        const struct early_mount *m = &early_mounts[i];

        if (mkdir(m->target, 0755) != 0 && errno != EEXIST)
            return -errno;

        if (mount("none", m->target, m->fstype, m->flags, m->options) != 0 && errno != EBUSY) {
            int err = -errno;
            open_console();
            LOG_ERR("Could not mount %s (%i)", m->target, err);
            return err;
        }
    }

    open_console();
    return 0;
}

int ks_mount_root(const char *device, const char *fstype, const char *options)
{
    if (mkdir(KS_NEWROOT, 0755) != 0 && errno != EEXIST)
        return -errno;

    if (mount(device, KS_NEWROOT, fstype, MS_RDONLY, options) != 0) {
        int err = -errno;
        LOG_ERR("Could not mount %s (%s) on " KS_NEWROOT " (%i)", device, fstype, err);
        return err;
    }

    return 0;
}

/* Delete everything in the directory 'dir_fd' that is on the device 'dev',
 * mount points are on other devices and are kept */
static void delete_contents(int dir_fd, dev_t dev)
{
    DIR *dir = fdopendir(dir_fd);
    struct dirent *d;

    if (dir == NULL) {
        close(dir_fd);
        return;
    }

    while ((d = readdir(dir)) != NULL) {
        struct stat st;

        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue;

        if (fstatat(dir_fd, d->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0 || st.st_dev != dev)
            continue;

        if (S_ISDIR(st.st_mode)) {
            int fd = openat(dir_fd, d->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

            if (fd >= 0)
                delete_contents(fd, dev);

            (void)unlinkat(dir_fd, d->d_name, AT_REMOVEDIR);
        } else {
            (void)unlinkat(dir_fd, d->d_name, 0);
        }
    }

    closedir(dir);
}

static int delete_initramfs(void)
{
    struct statfs sfs;
    struct stat st;
    int fd;

    /* Never delete anything unless / is the initramfs */
    if (statfs("/", &sfs) != 0 || stat("/", &st) != 0)
        return -errno;

    if (sfs.f_type != RAMFS_MAGIC && sfs.f_type != TMPFS_MAGIC) {
        LOG_ERR("/ is not an initramfs, not deleting it");
        return -EPERM;
    }

    fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (fd < 0)
        return -errno;

    delete_contents(fd, st.st_dev);
    return 0;
}

int ks_switch_root(const char *init, char **argv)
{
    char path[512];
    int rc;

    snprintf(path, sizeof(path), KS_NEWROOT "%s", init);

    if (access(path, X_OK) != 0) {
        rc = -errno;
        LOG_ERR("%s is not executable in the root file system (%i)", init, rc);
        return rc;
    }

    for (size_t i = 0; i < KS_ARRAY_SIZE(early_mounts); i++) {
        const char *target = early_mounts[i].target;

        snprintf(path, sizeof(path), KS_NEWROOT "%s", target);

        if (mount(target, path, NULL, MS_MOVE, NULL) != 0) {
            LOG_WARN("Could not move %s to the root file system (%i)", target, -errno);
            (void)umount2(target, MNT_DETACH);
        }
    }

    if (chdir(KS_NEWROOT) != 0)
        return -errno;

    rc = delete_initramfs();

    if (rc != 0)
        return rc;

    if (mount(".", "/", NULL, MS_MOVE, NULL) != 0) {
        rc = -errno;
        LOG_ERR("Could not move " KS_NEWROOT " to / (%i)", rc);
        return rc;
    }

    if (chroot(".") != 0 || chdir("/") != 0) {
        rc = -errno;
        LOG_ERR("chroot failed (%i)", rc);
        return rc;
    }

    LOG_INFO("Starting %s", init);

    argv[0] = (char *)init;
    execv(init, argv);

    rc = -errno;
    LOG_ERR("Could not execute %s (%i)", init, rc);
    return rc;
}
