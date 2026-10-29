#!/bin/sh
#
# Kickstart
#
# Copyright (C) 2026 Jonas Blixt <jonas@blxt.se>
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Create a signed root image for kickstart: a bpak with a file system, its
# merkle hash tree for dm-verity and the root hash in the signed header.
#
# mkroot.sh [-p part] [-u package-uuid] <fs image> <private key> <keystore id> \
#           <key id> <output.bpak>
#
#   fs image     squashfs (or another read only file system), the size must be
#                a multiple of 4 KiB, mksquashfs pads to 4 KiB by default
#   private key  key to sign with, its public key must be in the keystore that
#                kickstart is built with
#   keystore id  keystore-provider-id of the keystore, a name or 0x<id>
#   key id       name of the key in the keystore, or 0x<id>
#   -p part      part name, KICKSTART_ROOT_PART, default rootfs
#   -u uuid      bpak-package uuid of the image
#
# The file system and the tree are excluded from the payload hash, kickstart
# only verifies the header at boot and dm-verity verifies each block when it
# is read.
#
set -eu

BPAK=${BPAK:-bpak}
PART=rootfs
UUID=

while getopts p:u: opt; do
    case $opt in
    p) PART=$OPTARG ;;
    u) UUID=$OPTARG ;;
    *) exit 2 ;;
    esac
done
shift $((OPTIND - 1))

if [ $# -ne 5 ]; then
    sed -n '10,27p' "$0"
    exit 2
fi

FS=$1
KEY=$2
KEYSTORE_ID=$3
KEY_ID=$4
OUT=$5

if [ $(($(stat -c %s "$FS") % 4096)) -ne 0 ]; then
    echo "$FS: the size is not a multiple of 4096 bytes" >&2
    exit 1
fi

$BPAK create "$OUT" -Y
if [ -n "$UUID" ]; then
    $BPAK add "$OUT" --meta bpak-package --from-string "$UUID" --encoder uuid
fi
$BPAK add "$OUT" --part "$PART" --from-file "$FS" --encoder merkle --set-flag dont-hash
$BPAK set "$OUT" --key-id "$KEY_ID" --keystore-id "$KEYSTORE_ID"
$BPAK sign "$OUT" --key "$KEY"
