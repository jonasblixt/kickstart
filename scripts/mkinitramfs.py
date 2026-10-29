#!/usr/bin/env python3
#
# Kickstart
#
# Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""Create the initramfs of kickstart, a cpio archive in the "newc" format.

The archive has /init, /dev/console and the directories that kickstart
mounts on. Device nodes are written to the archive directly, no root
permissions are needed. All files are owned by root.

    mkinitramfs.py --init build/kickstart -o initramfs.cpio.gz
"""

import argparse
import gzip
import os
import stat
import sys

DIRECTORIES = ["dev", "proc", "sys", "run", "newroot"]


class Cpio:
    def __init__(self) -> None:
        self.data = bytearray()
        self.ino = 1

    def _entry(self, name: str, mode: int, body: bytes = b"", rdev: tuple[int, int] = (0, 0)):
        name_bytes = name.encode() + b"\0"
        fields = [
            self.ino,  # ino
            mode,
            0,  # uid
            0,  # gid
            2 if stat.S_ISDIR(mode) else 1,  # nlink
            0,  # mtime, reproducible archives
            len(body),
            0,  # devmajor
            0,  # devminor
            rdev[0],
            rdev[1],
            len(name_bytes),
            0,  # check
        ]
        self.ino += 1
        self.data += b"070701" + b"".join(b"%08X" % f for f in fields)
        self.data += name_bytes
        self.data += b"\0" * (-len(self.data) % 4)
        self.data += body
        self.data += b"\0" * (-len(self.data) % 4)

    def directory(self, name: str) -> None:
        self._entry(name, stat.S_IFDIR | 0o755)

    def file(self, name: str, body: bytes, perm: int) -> None:
        self._entry(name, stat.S_IFREG | perm, body)

    def char_device(self, name: str, major: int, minor: int, perm: int) -> None:
        self._entry(name, stat.S_IFCHR | perm, rdev=(major, minor))

    def finish(self) -> bytes:
        self._entry("TRAILER!!!", 0)
        self.data += b"\0" * (-len(self.data) % 512)
        return bytes(self.data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--init", required=True, help="the kickstart binary")
    parser.add_argument("-o", "--output", required=True, help="output, gzip compressed for .gz")
    args = parser.parse_args()

    with open(args.init, "rb") as f:
        init = f.read()

    cpio = Cpio()
    cpio.directory(".")
    for d in DIRECTORIES:
        cpio.directory(d)
    cpio.char_device("dev/console", 5, 1, 0o600)
    cpio.file("init", init, 0o755)
    archive = cpio.finish()

    if args.output.endswith(".gz"):
        archive = gzip.compress(archive, mtime=0)

    with open(args.output, "wb") as f:
        f.write(archive)

    return 0


if __name__ == "__main__":
    sys.exit(main())
