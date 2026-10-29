#!/bin/sh
#
# Create the test images: a keystore with one key and root images with a
# squashfs file system and its merkle tree, signed and broken in different
# ways.
#
# mkfixtures.sh <bpak tool> <output directory>
#
set -eu

BPAK=$1
OUT=$2

mkdir -p "$OUT"
cd "$OUT"

openssl ecparam -name prime256v1 -genkey -noout -out key.pem 2>/dev/null
openssl ec -in key.pem -pubout -out key-pub.pem 2>/dev/null
openssl ecparam -name prime256v1 -genkey -noout -out other-key.pem 2>/dev/null

# The keystore, the keystore id is bpak_id("kickstart-test") and the key id
# bpak_id("test-key")
$BPAK create keystore.bpak -Y
$BPAK add keystore.bpak --meta bpak-package --from-string 1d0c4d2c-0f0a-4b3e-9b65-6b0a41f0a7c1 \
    --encoder uuid
$BPAK add keystore.bpak --meta keystore-provider-id --from-string kickstart-test --encoder id
$BPAK add keystore.bpak --part test-key --from-file key-pub.pem --encoder key

rm -rf rootfs-dir
mkdir -p rootfs-dir/sbin rootfs-dir/dev rootfs-dir/proc rootfs-dir/sys rootfs-dir/run
printf '#!/bin/sh\necho kickstart test root\n' > rootfs-dir/sbin/init
chmod 755 rootfs-dir/sbin/init
dd if=/dev/urandom of=rootfs-dir/data bs=4096 count=200 2>/dev/null
rm -f rootfs.squashfs
mksquashfs rootfs-dir rootfs.squashfs -quiet -no-progress -all-root -noappend >/dev/null

# root image <file> <key> [part flags] [extra part before the file system]
root_image() {
    $BPAK create "$1" -Y
    $BPAK add "$1" --meta bpak-package --from-string 8c4a8b5e-2d46-4bb4-a9b7-58a0b0b9b3c4 \
        --encoder uuid
    if [ -n "${4:-}" ]; then
        # A part that is not a multiple of 4 KiB, the file system starts at an
        # offset that is only 512 byte aligned
        head -c 1000 /dev/urandom > extra.bin
        $BPAK add "$1" --part extra --from-file extra.bin
    fi
    $BPAK add "$1" --part rootfs --from-file rootfs.squashfs --encoder merkle $3
    $BPAK set "$1" --key-id test-key --keystore-id kickstart-test
    $BPAK sign "$1" --key "$2"
}

root_image rootfs.bpak key.pem "--set-flag dont-hash"
root_image rootfs-hashed.bpak key.pem ""
root_image rootfs-unaligned.bpak key.pem "--set-flag dont-hash" extra
root_image rootfs-other-key.bpak other-key.pem "--set-flag dont-hash"

# Header changed after signing, a byte in the meta data array
cp rootfs.bpak rootfs-bad-header.bpak
printf '\377' | dd of=rootfs-bad-header.bpak bs=1 seek=1100 conv=notrunc 2>/dev/null

# A hashed part changed after signing, the payload hash fails
cp rootfs-hashed.bpak rootfs-bad-payload.bpak
printf '\377' | dd of=rootfs-bad-payload.bpak bs=1 seek=8192 conv=notrunc 2>/dev/null

# Missing the end of the hash tree
size=$(stat -c %s rootfs.bpak)
head -c $((size - 4096)) rootfs.bpak > rootfs-truncated.bpak
