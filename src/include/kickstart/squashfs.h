#ifndef __SQUASHFS_H__
#define __SQUASHFS_H__

#define DEFAULT_VERITY_DATA_BLOCK 4096
#define DEFAULT_VERITY_HASH_BLOCK 4096
#define DEFAULT_VERITY_SALT_SIZE 32
#define DEFAULT_VERITY_HASH "sha256"

typedef long long		squashfs_block;
typedef long long		squashfs_inode;

struct squashfs_super_block
{
	unsigned int		s_magic;
	unsigned int		inodes;
	int			        mkfs_time;
	unsigned int		block_size;
	unsigned int		fragments;
	unsigned short		compression;
	unsigned short		block_log;
	unsigned short		flags;
	unsigned short		no_ids;
	unsigned short		s_major;
	unsigned short		s_minor;
	squashfs_inode		root_inode;
	long long		bytes_used;
	long long		id_table_start;
	long long		xattr_id_table_start;
	long long		inode_table_start;
	long long		directory_table_start;
	long long		fragment_table_start;
	long long		lookup_table_start;
};

#define KICKSTART_BLOCK_MAGIC 0x64c18c1e
#define KICKSTART_BLOCK_VERSION 1
#define KICKSTART_BLOCK_SIG_MAX_SIZE 256

struct kickstart_block
{
    uint32_t magic;
    uint8_t version;
    uint8_t signature[KICKSTART_BLOCK_SIG_MAX_SIZE];
    uint8_t salt[32];
    uint8_t hash[32];
    uint8_t key_index;
    uint8_t hash_kind;
    uint8_t sign_kind;
    uint16_t sign_length;
    uint8_t rz[182];
} __attribute__ ((packed));

#endif
