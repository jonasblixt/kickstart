#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <uuid.h>
#include <blkid.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <libcryptsetup.h>
#include <stdbool.h>

#define NL_MAX_PAYLOAD 8192

#define ks_log(...) \
    do { FILE *fp = fopen("/dev/kmsg","w"); \
    fprintf(fp, "ks: " __VA_ARGS__); \
    fclose(fp); } while(0)

static blkid_cache bc;

static void ks_panic(const char *msg)
{
    printf ("Panic: %s, rebooting in 1s...\n", msg);
    ks_log ("Panic: %s, rebooting in 1s...\n", msg);
    sleep(1);
    reboot(0x1234567);
    while (1);
}

static int ks_readfile(const char *fn, char *buf, size_t sz)
{
    FILE *fp = fopen(fn,"r");

    if (fp == NULL)
        return -1;
    memset(buf, 0, sz);
    int result = fread (buf,1,sz,fp);

    fclose(fp);

    return (result > 0)?0:-1;
}

static void ks_wait_for_device(const char *devstring)
{
    int nl_socket;
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr* nlh;
    char msg[NL_MAX_PAYLOAD];

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = -1;

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;

    nlh = (struct nlmsghdr*) malloc(NLMSG_SPACE(NL_MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(NL_MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(NL_MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    nl_socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

    if (nl_socket < 0)
    {
        printf("Failed to create socket for DeviceFinder");
        exit(1);
    }

    bind(nl_socket, (struct sockaddr*) &src_addr, sizeof(src_addr));

    while (1)
    {
        int r = recv(nl_socket, msg, sizeof(msg), MSG_DONTWAIT);
        if (r >= 0)
        {
            if (strstr (msg, devstring) != NULL)
            {
                break;
            }
        }
    }

    close(nl_socket);
}

static ssize_t hex_to_bytes(const char *hex, char **result)
{
	char buf[3] = "xx\0", *endp, *bytes;
	size_t i, len;

	len = strlen(hex);

	if (len % 2)
		return -1;

	len /= 2;

	bytes = malloc(len);

	if (!bytes)
    {
		return -1;
    }

	for (i = 0; i < len; i++)
    {
		memcpy(buf, &hex[i * 2], 2);

		bytes[i] = strtoul(buf, &endp, 16);

		if (endp != &buf[2])
        {
			free(bytes);
			return -1;
		}
	}

	*result = bytes;
	return i;
}

static int ks_get_root_device(char **root_device_str, char active_system)
{
    int rc;

    rc = blkid_get_cache(&bc, "/dev/null");
    
    if (rc != 0)
        return -1;


    if (active_system == 'A')
    {
        *root_device_str = 
            blkid_get_devname(bc,"PARTUUID","c284387a-3377-4c0f-b5db-1bcbcff1ba1a");
    }
    else if (active_system == 'B')
    {
        *root_device_str = 
            blkid_get_devname(bc,"PARTUUID","ac6a1b62-7bd0-460b-9e6a-9a7831ccbfbb");
    }
    else
    {
        ks_log ("No active system\n");
        return -1;
    }

    return 0;
}

static void ks_early_init(void)
{
    int rc;

    rc = mount("none", "/proc", "proc", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount /proc\n");

    rc = mount("none", "/sys", "sysfs", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount /sys\n");

    rc = mount("none", "/dev", "devtmpfs", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount /dev\n");
}

static int ks_verity_init(char *root_device_str, char *root_hash_string)
{
	struct crypt_device *cd = NULL;
	struct crypt_params_verity params = {};
	uint32_t activate_flags = CRYPT_ACTIVATE_READONLY;
    const char *delim = ":";
    int rc;

    if (crypt_init_data_device(&cd, root_device_str,root_device_str) != 0)
        return -1;

    uint64_t hash_offset = strtol(strtok(root_hash_string,delim),NULL,0);

    ks_log ("Hash offset: %lu\n",hash_offset);

    params.flags = 0;
    params.hash_area_offset = hash_offset;
    params.fec_area_offset = 0;
    params.fec_device = NULL;
    params.fec_roots = 0;

    if (crypt_load(cd, CRYPT_VERITY, &params) != 0)
        return -1;

	ssize_t hash_size = crypt_get_volume_key_size(cd);
    char *root_hash_bytes = NULL;
    char *root_hash_str = strtok(NULL, delim);
    ks_log ("root hash: %s\n",root_hash_str);

    hex_to_bytes(root_hash_str, &root_hash_bytes);

	rc = crypt_activate_by_volume_key(cd, "vroot",
					 root_hash_bytes,
					 hash_size,
					 activate_flags);

    if (rc != 0)
        return -1;

    return 0;
}

static int ks_switchroot(const char *root_device, const char *fs_type)
{
    int rc;

    rc = mount(root_device, "/newroot", fs_type, MS_RDONLY, "");

    if (rc == -1)
      ks_panic ("Could not mount /newroot");

    mount("/dev",  "/newroot/dev", NULL, MS_MOVE, NULL);
    mount("/proc", "/newroot/proc", NULL, MS_MOVE, NULL);
    mount("/sys",  "/newroot/sys", NULL, MS_MOVE, NULL);

    rc = chdir("/newroot");

    if (rc != 0)
    { 
        ks_log("Could not change to /newroot");
        return -1;
    }

	if (mount("/newroot", "/", NULL, MS_MOVE, NULL) < 0)
    {
        ks_log ("Could not remount newroot\n");
        perror("Mount new root");
        return -1;
    }

    rc = chroot(".");

    if (rc != 0)
    {
        ks_log("Could not chroot\n");
        return -1;
    }

	pid_t pid = fork();

	if (pid <= 0)
    {
        ks_log ("clean-up...");

        /* Remove files from initrd */
        unlink("/init");
        unlink("/roothash");

        if (pid == 0)
            exit(0);
    }
    
    return 0;
}

#define ACTIVE_SYSTEM_BUF_SZ 16
#define ROOT_HASH_BUF_SZ 128

int main(int argc, char **argv)
{
    char active_system[ACTIVE_SYSTEM_BUF_SZ];
    char root_hash_string[ROOT_HASH_BUF_SZ];
    char *root_device_str = NULL;

    /* Initialize really early stuff, like mounting /proc, /sys etc */
    ks_early_init();

    ks_log("Kickstart " VERSION " starting...\n");

    /* Read information about which root system we are going to
     * try to mount. ks-initrd currently only supports the punchboot
     * boot loader */
    if (ks_readfile("/proc/device-tree/chosen/active-system",
                    active_system, ACTIVE_SYSTEM_BUF_SZ) != 0)
    {
        ks_panic("Could not read active-system\n");
    }

    /* Load root volume hash and hash offset */
    if (ks_readfile("/roothash", root_hash_string, ROOT_HASH_BUF_SZ) != 0)
    {
        ks_panic("Could not read root hash\n");
    }

    ks_log("Active System: %s\n",active_system);
    ks_log("Waiting for %s\n", ROOTDEVICE);

    /* Wait for root block device to become available */
    /* HACK: for emmc's boot devices enumerates after all partitions
     *  this should be handeled in a more generic way */
    ks_wait_for_device(ROOTDEVICE"boot0");

    /* Lookup partition UUID and translate to a /dev device-node */
    if (ks_get_root_device(&root_device_str, active_system[0]) != 0)
        ks_panic("Could not get root device\n");

    ks_log ("Using root device: %s\n", root_device_str);

    /* Initialize dm-verity mapper volume */
    if (ks_verity_init(root_device_str, root_hash_string) != 0)
        ks_panic("Could not initialize verity device\n");

    if (ks_switchroot("/dev/mapper/vroot", "squashfs") != 0)
        ks_panic ("Could not activate new system\n");

    ks_log("Starting real init...\n");

	execv("/kickstart", argv);

    /* Sould not be reached */
    ks_panic("Init returned");
    while (1);
}
