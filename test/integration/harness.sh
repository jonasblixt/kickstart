#!/bin/sh
#
# Runs in the privileged container of run.sh. Each case builds an initramfs
# on a tmpfs, attaches a root image to a loop device and runs kickstart as
# PID 1 of a new PID namespace, chrooted to the initramfs.
#
set -u

apk add -q util-linux device-mapper >/dev/null

# The /dev of the container is a copy from when it started, the nodes of
# the loop and device mapper devices that are created later are only in the
# devtmpfs. It is the instance of the host, kickstart mounts it as well and
# creates the /dev/mapper nodes there.
mount -t devtmpfs none /dev

LOOP=
FAILED=0

cleanup() {
    for name in kickstart-root kickstart-root-hash kickstart-root-data; do
        if dmsetup info "$name" >/dev/null 2>&1; then
            dmsetup remove --retry "$name" || echo "harness: could not remove $name"
        fi
        rm -f "/dev/mapper/$name"
    done

    if [ -n "$LOOP" ]; then
        losetup -d "$LOOP"
        LOOP=
    fi

    umount /initramfs 2>/dev/null
}

trap cleanup EXIT

initramfs() {
    mkdir -p /initramfs
    mount -t tmpfs -o mode=0755 none /initramfs
    mkdir -p /initramfs/dev /initramfs/proc /initramfs/sys /initramfs/run /initramfs/newroot
    mkdir -p /initramfs/ks-test/chosen
    cp /ks-test-src/kickstart /initramfs/init
    cp /ks-test-src/chosen/* /initramfs/ks-test/chosen/
    cp /ks-test-src/pbstate-primary /ks-test-src/pbstate-backup /initramfs/ks-test/

    if ! LOOP=$(losetup -f --show -r "/images/$1"); then
        echo "harness: losetup failed"
        exit 1
    fi
    set -- $(stat -c '%t %T' "$LOOP")
    mknod /initramfs/ks-test/root-a b $((0x$1)) $((0x$2))
}

# boot <image> <expected output>
boot() {
    echo "-- $1"
    initramfs "$1"

    # Not PID 1, a dry run that prints the verity table. The host restarts
    # on corruption unless the init is built with 'eio'.
    if chroot /initramfs /init 2>&1 | grep -q "_on_corruption"; then
        echo "harness: the test init must be built with KICKSTART_VERITY_ON_CORRUPTION=eio"
        exit 1
    fi

    unshare --pid --fork --mount --propagation private chroot /initramfs /init \
        >/tmp/log 2>&1
    sed 's/^/   /' /tmp/log

    if grep -q "$2" /tmp/log && ! grep -q "KICKSTART-INTEGRATION-FAIL" /tmp/log; then
        echo "PASS $1"
    else
        echo "FAIL $1, expected '$2'"
        FAILED=1
    fi

    cleanup
}

boot good.bpak KICKSTART-INTEGRATION-OK
boot corrupt.bpak KICKSTART-INTEGRATION-EIO
boot bad-sig.bpak "Boot failed"

exit $FAILED
