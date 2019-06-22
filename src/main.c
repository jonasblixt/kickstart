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
#include <sys/socket.h>
#include <linux/netlink.h>
#include <libcryptsetup.h>

#define NL_MAX_PAYLOAD 8192

#ifndef PB_PARTUUID_SYSTEM_A
    #define PB_PARTUUID_SYSTEM_A "\x2a\xf7\x55\xd8\x8d\xe5\x45\xd5\xa8\x62\x01\x4c\xfa\x73\x5c\xe0"
#endif

#ifndef PB_PARTUUID_SYSTEM_B
    #define PB_PARTUUID_SYSTEM_B "\xc0\x46\xcc\xd8\x0f\x2e\x40\x36\x98\x4d\x76\xc1\x4d\xc7\x39\x92"
#endif

#ifndef PB_PARTUUID_ROOT_A
    #define PB_PARTUUID_ROOT_A "\xc2\x84\x38\x7a\x33\x77\x4c\x0f\xb5\xdb\x1b\xcb\xcf\xf1\xba\x1a"
#endif

#ifndef PB_PARTUUID_ROOT_B
    #define PB_PARTUUID_ROOT_B "\xac\x6a\x1b\x62\x7b\xd0\x46\x0b\x9e\x6a\x9a\x78\x31\xcc\xbf\xbb"
#endif

#ifndef PB_PARTUUID_CONFIG_PRIMARY
    #define PB_PARTUUID_CONFIG_PRIMARY "\xf5\xf8\xc9\xae\xef\xb5\x40\x71\x9b\xa9\xd3\x13\xb0\x82\x28\x1e"
#endif

#ifndef PB_PARTUUID_CONFIG_BACKUP
    #define PB_PARTUUID_CONFIG_BACKUP "\x65\x6a\xb3\xfc\x58\x56\x4a\x5e\xa2\xae\x5a\x01\x83\x13\xb3\xee"
#endif


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

int main(int argc, char **argv)
{
    uint64_t offset_primary, offset_backup;
    int rc;
    

    rc = mount("none", "/proc", "proc", 0, "");

    if (rc == -1)
    {
        printf ("Could not mount /proc\n");
        exit(-1);
    }


    rc = mount("none", "/sys", "sysfs", 0, "");

    if (rc == -1)
    {
        printf ("Could not mount /sys\n");
        exit(-1);
    }

    rc = mount("none", "/dev", "devtmpfs", 0, "");

    if (rc == -1)
    {
        printf ("Could not mount /dev\n");
        exit(-1);
    }

    FILE *log = fopen("/dev/kmsg","w");
    fprintf (log,"Kickstart %s starting...\n", VERSION);
    fclose(log);
    char active_system[16];
    char device_uuid[37];

    FILE *fp = fopen("/proc/device-tree/chosen/active-system","r");
    fread (active_system,16,1,fp);
    fclose(fp);

    fp = fopen("/proc/device-tree/chosen/device-uuid","r");
    fread (device_uuid,37,1,fp);
    fclose(fp);

    //printf("Active System: %s\n",active_system);
    //printf("Device UUID: %s\n",device_uuid);

    struct termios ctrl;

    tcgetattr(STDIN_FILENO, &ctrl);
    ctrl.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

   int nl_socket;
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr* nlh;
    char msg[NL_MAX_PAYLOAD];

    // Prepare source address
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = -1;

    // Prepare destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;

    // Prepare netlink message
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

    //printf ("Waiting for eMMC...\n");
    while (1) {
        int r = recv(nl_socket, msg, sizeof(msg), MSG_DONTWAIT);
        if (r >= 0)
        {
            //printf ("%s\n",msg);
            if (strcmp (msg, "bind@/devices/platform/5b010000.usdhc/mmc_host/mmc0/mmc0:0001") == 0)
                break;
        }
    }


    blkid_probe pr;
    pr = blkid_new_probe_from_filename("/dev/mmcblk0");

    if (!pr)
    {
        printf("Error: could not probe '/dev/mmcblk0'\n");
        return -1;
    }
    
    blkid_do_probe(pr);
    blkid_partlist pl = blkid_probe_get_partitions(pr);
    int no_of_parts = blkid_partlist_numof_partitions(pl);

    for (uint32_t n = 0; n < no_of_parts; n++)
    {
        blkid_partition p = blkid_partlist_get_partition(pl,n);
        uuid_t uuid_raw;
        uuid_parse(blkid_partition_get_uuid(p), uuid_raw);

        if (memcmp(uuid_raw, PB_PARTUUID_CONFIG_PRIMARY, 16) == 0)
        {
            offset_primary = blkid_partition_get_start(p);
            //printf ("Found primary configuration at lba %lx\n",offset_primary);
            //printf ("name: %s\n",blkid_partition_get_name(p));
        }

        if (memcmp(uuid_raw, PB_PARTUUID_CONFIG_BACKUP, 16) == 0)
        {
            offset_backup = blkid_partition_get_start(p);
            //printf ("Found backup configuration at lba %lx\n",offset_backup);
        }

    }

	struct crypt_device *cd = NULL;
	struct crypt_params_verity params = {};
	uint32_t activate_flags = CRYPT_ACTIVATE_READONLY;
    //fprintf (log,"Calling crypt_init_data_device\n");
    rc = crypt_init_data_device(&cd, "/dev/mmcblk0p3", "/dev/mmcblk0p3");

    //fprintf (log,"crypt_init_data_device = %i\n",rc);
    params.flags = 0;
    params.hash_area_offset = 6533120;
    params.fec_area_offset = 0;
    params.fec_device = NULL;
    params.fec_roots = 0;

    rc = crypt_load(cd, CRYPT_VERITY, &params);

    //fprintf (log,"crypt_load = %i\n",rc);

	ssize_t hash_size = crypt_get_volume_key_size(cd);
    char *root_hash_bytes = NULL;
    hex_to_bytes("5268cdd7cdabf1cf180a931e4946b5e09cae9735c268f8c8f7a8ff0e303a8523",
                &root_hash_bytes);

	rc = crypt_activate_by_volume_key(cd, "vroot",
					 root_hash_bytes,
					 hash_size,
					 activate_flags);
    //fprintf (log,"crypt_activate_by_volume = %i\n",rc);
/*
    while (1) {
        int r = recv(nl_socket, msg, sizeof(msg), MSG_DONTWAIT);
        if (r >= 0)
        {
            printf("uevt: %i %s\n", r, msg);
        }
    }*/

    rc = mount("/dev/mapper/vroot", "/newroot", "squashfs", MS_RDONLY, "");

    if (rc == -1)
    {
      printf ("Could not mount /newroot %i\n",rc);
      perror("Err:");
    }

    umount("/dev");
    mount("/newroot/proc", "/proc", NULL, MS_MOVE, NULL);
    mount("/newroot/sys", "/sys", NULL, MS_MOVE, NULL);

    rc = mount("none", "/newroot/dev", "devtmpfs", 0, "");

    if (rc == -1)
    {
        printf ("Could not mount /dev\n");
        exit(-1);
    }
    chdir("/newroot");

	if (mount("/newroot", "/", NULL, MS_MOVE, NULL) < 0)
    {
        printf ("Could not remount newroot\n");
        perror("Mount new root");
    }


    chroot(".");

	pid_t pid = fork();
	if (pid <= 0)
    {
        //fprintf (log,"ks-init: cleaning up...\n");
        if (pid == 0)
            exit(0);
    }


/*
    DIR *d;
    struct dirent *dir;
    d = opendir("/dev/");

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }

        closedir(d);
    }
*/

    log = fopen("/dev/kmsg","w");
    fprintf (log,"Kickstart done...\n", VERSION);
    fclose(log);
	execv("/sbin/init", argv);
    perror("execv returned");
}
