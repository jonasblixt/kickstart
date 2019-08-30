#!/bin/sh

D=/tmp/rootfs

mkdir -p $D/dev
mkdir -p $D/sys
mkdir -p $D/proc
mkdir -p $D/lib

cp /sdk/kickstart $D/
cp /sdk/lib/ld-linux-aarch64.so.1 $D/lib
cp /sdk/lib/libc.so.6 $D/lib
cp /sdk/lib/libcryptsetup.so.12 $D/lib
cp /sdk/lib/libuuid.so.1 $D/lib
cp /sdk/lib/libdevmapper.so.1.02 $D/lib
cp /sdk/lib/libjson-c.so.4 $D/lib
cp /sdk/lib/libm.so.6 $D/lib
cp /sdk/lib/libpthread.so.0 $D/lib
cp /sdk/lib/libaio.so $D/lib
cp /sdk/lib/libblkid.so.1 $D/lib

rm -f /tmp/root.squash
mksquashfs $D /tmp/root.squash -noI -noD -noF -noX
veritysetup format /tmp/root.squash /tmp/root.hash > /tmp/verity.log

echo "veritysetup output:" 
cat /tmp/verity.log
ROOT_SALT=$(cat /tmp/verity.log |  grep -oP "^Salt.*\K([a-z0-9]{64})")

cp /tmp/root.squash /pkg/build/
cp /tmp/root.hash /pkg/build
echo -n $ROOT_SALT > /pkg/build/root.salt

