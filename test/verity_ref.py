#!/usr/bin/env python3
#
# Check that the merkle tree of a bpak part is the hash tree that dm-verity
# expects. The tree is computed here the way the kernel reads it
# (drivers/md/dm-verity-target.c, format version 1, sha256, 4 KiB blocks),
# without bpak, and compared with the tree part and the root hash meta data.
#
# verity_ref.py <package.bpak> <part name>
#

import hashlib
import struct
import sys

BLOCK = 4096
HASH = 32
HEADER = 4096
META_OFFSET = 8
PARTS_OFFSET = META_OFFSET + 32 * 16
METADATA_OFFSET = PARTS_OFFSET + 32 * 32

MERKLE_SALT = 0x7C9B2F93
MERKLE_ROOT_HASH = 0xE68FC9BE


def bpak_id(name: str) -> int:
    # CRC32 without the initial and final inversion
    crc = 0
    for b in name.encode():
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return crc


def parse(pkg: bytes):
    parts = {}
    offset = HEADER
    for i in range(32):
        pid, size, _, _, pad, flags, _ = struct.unpack_from(
            "<IQQQHBB", pkg, PARTS_OFFSET + i * 32
        )
        if pid == 0:
            break
        parts[pid] = (offset, size)
        offset += size + pad

    meta = {}
    for i in range(32):
        mid, size, moff, ref = struct.unpack_from("<IHHI", pkg, META_OFFSET + i * 16)
        if mid == 0:
            break
        start = METADATA_OFFSET + moff
        meta[(mid, ref)] = pkg[start : start + size]

    return parts, meta


def verity_tree(data: bytes, salt: bytes):
    def h(block: bytes) -> bytes:
        return hashlib.sha256(salt + block).digest()

    data_blocks = len(data) // BLOCK
    bits = (BLOCK // HASH).bit_length() - 1

    # Number of levels, as in verity_ctr()
    levels = 0
    while bits * levels < 64 and (data_blocks - 1) >> (bits * levels):
        levels += 1

    if levels == 0:
        return b"", h(data)

    level_data = [data]
    for _ in range(levels):
        src = level_data[-1]
        out = b"".join(h(src[i : i + BLOCK]) for i in range(0, len(src), BLOCK))
        out += b"\0" * (-len(out) % BLOCK)
        level_data.append(out)

    # The top level is first, the hashes of the data blocks are last
    tree = b"".join(reversed(level_data[1:]))
    return tree, h(level_data[-1])


def main() -> int:
    pkg = open(sys.argv[1], "rb").read()
    name = sys.argv[2]
    parts, meta = parse(pkg)

    data_off, data_size = parts[bpak_id(name)]
    tree_off, tree_size = parts[bpak_id(name + "-hash-tree")]
    salt = meta[(MERKLE_SALT, bpak_id(name))]
    root_hash = meta[(MERKLE_ROOT_HASH, bpak_id(name))]

    tree, root = verity_tree(pkg[data_off : data_off + data_size], salt)
    bpak_tree = pkg[tree_off : tree_off + tree_size]

    if root != root_hash:
        print(f"root hash {root.hex()} != {root_hash.hex()}")
        return 1

    if bpak_tree[: len(tree)] != tree:
        print("the hash tree does not match")
        return 1

    if any(bpak_tree[len(tree) :]):
        print("the hash tree has data after the dm-verity tree")
        return 1

    print(f"OK: {data_size // BLOCK} data blocks, tree {len(tree)} bytes, root {root.hex()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
