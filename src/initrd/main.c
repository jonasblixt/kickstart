#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <uuid.h>
#include <blkid.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <libcryptsetup.h>
#include <stdbool.h>
#include <kickstart/squashfs.h>
#include <kickstart/kickstart.h>
#include <3pp/bearssl/bearssl_hash.h>
#include <3pp/bearssl/bearssl_rsa.h>
#include <3pp/bearssl/bearssl_ec.h>
#include <3pp/bearssl/bearssl_x509.h>

#define NL_MAX_PAYLOAD 8192

#define ks_log2(...) \
    do { FILE *fp = fopen("/dev/kmsg","w"); \
    printf( "ks: " __VA_ARGS__); \
    fprintf(fp, "ks: " __VA_ARGS__); \
    fclose(fp); } while(0)

#define ks_log(...) \
    do { \
    printf( "ks: " __VA_ARGS__); \
    } while(0)

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

    rc = mount("none", "/tmp", "tmpfs", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount /tmp\n");

    rc = mount("none", "/data/tee", "tmpfs", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount /data/tee\n");

    rc = mount("none", "/sys/kernel/config", "configfs", 0, "");

    if (rc == -1)
        ks_panic ("Could not mount configfs\n");
}

static int ks_verity_init(char *root_device_str, uint8_t *root_hash,
                          uint64_t hash_offset)
{
	struct crypt_device *cd = NULL;
	struct crypt_params_verity params = {};
	uint32_t activate_flags = CRYPT_ACTIVATE_READONLY;
    int rc;

    if (crypt_init_data_device(&cd, root_device_str,root_device_str) != 0)
        return -1;

    params.flags = 0;
    params.hash_area_offset = hash_offset;
    params.fec_area_offset = 0;
    params.fec_device = NULL;
    params.fec_roots = 0;

    if (crypt_load(cd, CRYPT_VERITY, &params) != 0)
        return -1;

	ssize_t hash_size = crypt_get_volume_key_size(cd);

	rc = crypt_activate_by_volume_key(cd, "vroot",
					 (const char *) root_hash,
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
    mount("/tmp",  "/newroot/tmp", NULL, MS_MOVE, NULL);
    mount("/data/tee",  "/newroot/data/tee", NULL, MS_MOVE, NULL);
    mount("/sys/kernel/config",  "/newroot/sys/kernel/config", NULL, MS_MOVE, NULL);
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

        if (pid == 0)
            exit(0);
    }

    return 0;
}

#define KS_KEY_BUFFER_SZ 256

static int ks_init_device(const char *device_fn,
                          struct kickstart_block *ksb,
                          uint64_t *hash_tree_offset,
                          uint8_t *root_hash)
{
    struct squashfs_super_block sb;
    uint8_t signature[KICKSTART_BLOCK_SIG_MAX_SIZE];
    uint8_t key_buffer_asn1[KS_KEY_BUFFER_SZ];
    uint16_t sign_length = 0;
    char key_fn[16];
    uint64_t offset;
    uint8_t key_buf[160];
    uint8_t calc_hash[64];
    uint8_t hash_length = 0;
    br_sha256_context sha256_ctx;
    br_sha384_context sha384_ctx;
    br_sha512_context sha512_ctx;
    FILE *fp = fopen (device_fn, "r");

    if (fp == NULL)
    {
        ks_log ("Could not open device '%s'\n",device_fn);
        return KS_ERR;
    }

    if (!fread(&sb, sizeof(struct squashfs_super_block), 1, fp))
    {
        ks_log ("Could not read squashfs superblock\n");
        return KS_ERR;
    }

    if (sb.s_magic != 0x73717368)
    {
        ks_log("Incorrect squashfs magic\n");
        return KS_ERR;
    }

    offset = (sb.bytes_used + (4096 - (sb.bytes_used % 4096)));
    *hash_tree_offset = offset+512;

    if (fseek (fp, offset, SEEK_SET) != 0)
    {
        ks_log ("Seek failed\n");
        return KS_ERR;
    }

    if (!fread(ksb, sizeof(struct kickstart_block), 1, fp))
    {
        ks_log ("Could not read kickstart block\n");   
        return KS_ERR;
    }

    if (ksb->magic != KICKSTART_BLOCK_MAGIC)
    {
        ks_log("Incorrect kickstart magic\n");
        return KS_ERR;
    }

    fclose(fp);

    ks_log ("ksb offset: %lu bytes\n",offset);
    ks_log ("ksb magic: %x\n", ksb->magic);
    ks_log ("ksb version: %u\n",ksb->version);
    ks_log ("key_index = %u\n",ksb->key_index);

    memcpy(signature,ksb->signature, KICKSTART_BLOCK_SIG_MAX_SIZE);
    sign_length = ksb->sign_length;

    memset(ksb->signature,0, KICKSTART_BLOCK_SIG_MAX_SIZE);
    ksb->sign_length = 0;

    snprintf (key_fn,16,"/%u.der",ksb->key_index);

    br_ec_public_key pk;
    fp = fopen(key_fn, "r");

    if (fp == NULL)
    {
        ks_panic("Could not load key");
    }

    //size_t asn1_key_read_sz = fread(key_buf, 1, KS_KEY_BUFFER_SZ, fp);
    fclose(fp);

    const uint8_t k[] =
    {
        0x04, 0x39, 0x3D, 0xA9, 0x66, 0xF2, 0x08, 0x89, 0x6A, 0xC3, 0xAE, 0x37, 0x88, 0xF4, 0x09, 0xC8,
        0xB8, 0x1D, 0xCB, 0xD0, 0x6C, 0xA1, 0xCF, 0xB6, 0xAF, 0xE0, 0x3C, 0x65, 0x95, 0x19, 0x13, 0xAB,
        0xA7, 0x6C, 0x91, 0x0F, 0x55, 0xB6, 0xD4, 0xBC, 0x29, 0x07, 0xC8, 0x80, 0xD7, 0x91, 0x63, 0x15,
        0x06, 0xD3, 0x36, 0x6A, 0xDE, 0x2D, 0x30, 0x3D, 0xF1, 0x52, 0x96, 0xE3, 0x57, 0x35, 0x3F, 0xCF,
        0x0C, 0x25, 0x15, 0x56, 0x0F, 0xC6, 0x46, 0x5B, 0xBE, 0x88, 0x87, 0x32, 0x98, 0xDF, 0xE3, 0x47,
        0xFC, 0xB1, 0x6F, 0xBA, 0x06, 0x10, 0x4D, 0x2A, 0x08, 0xFC, 0xE8, 0xA3, 0x5E, 0xF2, 0xF2, 0x02,
        0xD9,
    };

    pk.q = k;
    pk.qlen = 97;

    ks_log("pk.qlen = %lu\n",pk.qlen);


    switch (ksb->sign_kind)
    {
        case KS_SIGN_NIST256p:
            br_sha256_init(&sha256_ctx);
            br_sha256_update(&sha256_ctx, ksb, sizeof(struct kickstart_block));
            br_sha256_out(&sha256_ctx, calc_hash);
            hash_length = 32;
            pk.curve = BR_EC_secp256r1;
        break;
        case KS_SIGN_NIST384p:
            br_sha384_init(&sha384_ctx);
            br_sha384_update(&sha384_ctx, ksb, sizeof(struct kickstart_block));
            br_sha384_out(&sha384_ctx, calc_hash);
            hash_length = 48;
            pk.curve = BR_EC_secp384r1;
        break;
        case KS_SIGN_NIST521p:
            br_sha512_init(&sha512_ctx);
            br_sha512_update(&sha512_ctx, ksb, sizeof(struct kickstart_block));
            br_sha512_out(&sha512_ctx, calc_hash);
            hash_length = 64;
            pk.curve = BR_EC_secp521r1;
        break;
        default:
        break;
    }

    struct timeval t,t2;

    gettimeofday(&t, NULL);
    if (br_ecdsa_i31_vrfy_asn1(&br_ec_prime_i31,
                                calc_hash,
                                hash_length,
                                &pk,
                                signature,
                                sign_length) != 1)
    {
        ks_log("Signature failed\n");
        return KS_ERR;
    }

    memcpy(root_hash, ksb->hash, 32);

    gettimeofday(&t2,NULL);

    printf ("Verification took %f us\n", (t2.tv_sec*1E6+t2.tv_usec)-
                                         (t.tv_sec*1E6+t.tv_usec));
    return KS_OK;
}

#define ACTIVE_SYSTEM_BUF_SZ 16

int main(int argc, char **argv)
{
    char active_system[ACTIVE_SYSTEM_BUF_SZ];
    char *root_device_str = NULL;
    uint64_t hash_tree_offset = 0;
    uint8_t root_hash[32];
    struct kickstart_block ksb;
    printf ("****** KS INIT *****\n");
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

    ks_log("Active System: %s\n",active_system);
    ks_log("Waiting for %s\n", ROOTDEVICE);

    /* Wait for root block device to become available */
    /* HACK: for emmc's boot devices enumerates after all partitions
     *  this should be handeled in a more generic way */
    /* TODO: Slower systems can enumerate mmc before init starts
     *  at which point there will be no message received */
    ks_wait_for_device(ROOTDEVICE"boot0");

    /* Lookup partition UUID and translate to a /dev device-node */
    if (ks_get_root_device(&root_device_str, active_system[0]) != 0)
        ks_panic("Could not get root device\n");

    ks_log ("Using root device: %s\n", root_device_str);

    if (ks_init_device(root_device_str, &ksb, &hash_tree_offset,
                        root_hash) != KS_OK)
    {
        ks_panic ("Not a valid ksfs");
    }

    /* Initialize dm-verity mapper volume */
    if (ks_verity_init(root_device_str, root_hash, hash_tree_offset) != 0)
        ks_panic("Could not initialize verity device\n");

    if (ks_switchroot("/dev/mapper/vroot", "squashfs") != 0)
        ks_panic ("Could not activate new system\n");

    ks_log("Starting real init...\n");

	execv("/kickstart", argv);

    /* Sould not be reached */
    ks_panic("Init returned");
    while (1);
}
