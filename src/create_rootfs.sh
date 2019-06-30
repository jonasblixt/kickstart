#!/bin/sh

D=/tmp/rootfs

mkdir -p $D/dev
mkdir -p $D/sys
mkdir -p $D/proc
mkdir -p $D/lib

cp /sdk/lib/ld-linux-aarch64.so.1 $D/lib
cp /sdk/lib/libc.so.6 $D/lib
cp -aR /input/* $D/

rm -f /tmp/root.squash
mksquashfs $D /tmp/root.squash -noI -noD -noF -noX
veritysetup format /tmp/root.squash /tmp/root.hash > /tmp/verity.log

echo "veritysetup output:" 
cat /tmp/verity.log
ROOT_HASH=$(cat /tmp/verity.log |  grep -oP "^Root.*\K([a-z0-9]{64})")
ROOTFS_SIZE=$(stat -c '%s' /tmp/root.squash)

cat /tmp/root.squash /tmp/root.hash > /pkg/build/rootfs.squash
echo -n $ROOTFS_SIZE:$ROOT_HASH > /ksbuild/build/roothash

