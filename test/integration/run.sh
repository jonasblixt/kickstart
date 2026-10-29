#!/bin/sh
#
# Kickstart
#
# Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
#
# SPDX-License-Identifier: BSD-3-Clause
#
# End to end test of the static init on the kernel of the build host:
# kickstart runs as PID 1 of a PID namespace in a privileged container,
# verifies a root image on a loop device, sets up dm-verity, mounts the
# squashfs and switches to its init.
#
# test/integration/run.sh <bpak source dir> <work dir>
#
# This changes the host while it runs: it loads the loop, dm-verity and
# squashfs modules if needed, creates loop and device mapper devices and
# /dev/mapper nodes in the host's devtmpfs, and kickstart logs to the host
# kernel log. Everything but the modules and the log is removed afterwards.
#
# The test init is built with KICKSTART_VERITY_ON_CORRUPTION=eio, the
# corruption test would restart the host with 'restart'.
#
set -eu

if [ $# -ne 2 ]; then
    sed -n '9,23p' "$0"
    exit 2
fi

BPAK_SRC=$(realpath "$1")
WORK=$(realpath -m "$2")
TOP=$(realpath "$(dirname "$0")/../..")
BPAK=${BPAK:-bpak}

rm -rf "$WORK/root" "$WORK/images" "$WORK/ks-test"
mkdir -p "$WORK/root/bin" "$WORK/root/sbin" "$WORK/images" "$WORK/ks-test/chosen"
for d in dev proc sys run; do mkdir -p "$WORK/root/$d"; done

echo "== Root file system"
docker run --rm -v "$WORK/root/bin:/out" alpine:3.22 sh -c \
    "apk add -q busybox-static && cp /bin/busybox.static /out/busybox && chown $(id -u):$(id -g) /out/busybox"

# 1 MiB of random data that does not compress, most of the file system
head -c 1048576 /dev/urandom > "$WORK/root/data"
sha256sum "$WORK/root/data" | cut -d' ' -f1 > "$WORK/root/data.sha256"
cp "$TOP/test/integration/root-init.sh" "$WORK/root/sbin/init"
chmod 755 "$WORK/root/sbin/init"
mksquashfs "$WORK/root" "$WORK/images/rootfs.squashfs" -quiet -no-progress -all-root -noappend \
    >/dev/null

echo "== Keys and images"
cd "$WORK/images"
openssl ecparam -name prime256v1 -genkey -noout -out key.pem 2>/dev/null
openssl ec -in key.pem -pubout -out key-pub.pem 2>/dev/null
openssl ecparam -name prime256v1 -genkey -noout -out other-key.pem 2>/dev/null
$BPAK create keystore.bpak -Y
$BPAK add keystore.bpak --meta bpak-package --from-string 1d0c4d2c-0f0a-4b3e-9b65-6b0a41f0a7c1 \
    --encoder uuid
$BPAK add keystore.bpak --meta keystore-provider-id --from-string kickstart-test --encoder id
$BPAK add keystore.bpak --part test-key --from-file key-pub.pem --encoder key
$BPAK generate keystore keystore.bpak --name kickstart > keystore.c

"$TOP/scripts/mkroot.sh" rootfs.squashfs key.pem kickstart-test test-key good.bpak
"$TOP/scripts/mkroot.sh" rootfs.squashfs other-key.pem kickstart-test test-key bad-sig.bpak

# A byte in the data file. It is random and does not compress, squashfs
# stores it as it is, find it in the image.
cp good.bpak corrupt.bpak
offset=$(python3 -I -c '
import sys
image = open(sys.argv[1], "rb").read()
data = open(sys.argv[2], "rb").read()
start = image.find(data[256 * 1024 : 256 * 1024 + 64])
assert start > 0, "the data file is not in the image"
print(start)' corrupt.bpak "$WORK/root/data")
printf '\377' | dd of=corrupt.bpak bs=1 seek="$offset" conv=notrunc 2>/dev/null

echo "== Static init"
mkdir -p "$WORK/build"
cp keystore.c "$WORK/build/keystore.c"
"$TOP/scripts/build-alpine.sh" "$WORK/build" "$BPAK_SRC" \
    -DKICKSTART_ROOT_A=/ks-test/root-a \
    -DKICKSTART_ROOT_B=/ks-test/root-b \
    -DKICKSTART_PBSTATE_PRIMARY=/ks-test/pbstate-primary \
    -DKICKSTART_PBSTATE_BACKUP=/ks-test/pbstate-backup \
    -DKICKSTART_CHOSEN=/ks-test/chosen \
    -DKICKSTART_VERITY_ON_CORRUPTION=eio \
    -DKICKSTART_DEVICE_TIMEOUT_MS=2000 \
    -DKICKSTART_LOG_LEVEL=debug \
    -DKICKSTART_KEYSTORE=/build/keystore.c >/dev/null
cp "$WORK/build/kickstart" "$WORK/ks-test/kickstart"

echo "== Device tree and pbstate"
python3 -I - "$WORK/ks-test" <<'EOF'
import struct, sys

out = sys.argv[1]

def bpak_id(name):
    crc = 0
    for b in name.encode():
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return crc

def prop(name, data):
    open(f"{out}/chosen/{name}", "wb").write(data)

prop("pb,active-system", b"A\0")
prop("pb,device-uuid", b"6f2c7c8e-6a4e-5b4c-8e2f-3c0d9a1b2c3d\0")
prop("pb,slc", struct.pack(">I", 3))
prop("pb,slc-active-key", struct.pack(">I", bpak_id("test-key")))
prop("pb,slc-available-keys", struct.pack(">II", 0xa90f9680, bpak_id("test-key")))

# struct pb_boot_state, A enabled and verified
state = bytearray(512)
struct.pack_into("<IIIII", state, 0, 0x026D4A65, 1, 1, 0, 0)
crc = 0
for b in state:
    crc ^= b
    for _ in range(8):
        crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
struct.pack_into("<I", state, 508, crc)
open(f"{out}/pbstate-primary", "wb").write(state)
open(f"{out}/pbstate-backup", "wb").write(state)
EOF

echo "== Boot"
docker run --rm --privileged \
    -v "$WORK/images:/images:ro" \
    -v "$WORK/ks-test:/ks-test-src:ro" \
    -v "$TOP/test/integration/harness.sh:/harness.sh:ro" \
    alpine:3.22 sh /harness.sh
