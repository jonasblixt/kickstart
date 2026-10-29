#!/bin/busybox sh
#
# /sbin/init of the integration test root file system. kickstart has
# switched to it, check the root and read the data file through dm-verity.
#
B=/bin/busybox

root=$($B awk '$2 == "/" { print $1, $3, $4 }' /proc/mounts | $B tail -n 1)
echo "root-init: / is $root"

case "$root" in
/dev/mapper/kickstart-root\ squashfs\ ro*) ;;
*)
    echo "KICKSTART-INTEGRATION-FAIL: unexpected root"
    exit 1
    ;;
esac

for d in /dev /proc /sys /run; do
    if ! $B mountpoint -q $d; then
        echo "KICKSTART-INTEGRATION-FAIL: $d is not mounted"
        exit 1
    fi
done

if [ -e /init ]; then
    echo "KICKSTART-INTEGRATION-FAIL: the initramfs is still visible"
    exit 1
fi

if ! sum=$($B sha256sum /data 2>/dev/null); then
    echo "KICKSTART-INTEGRATION-EIO"
    exit 0
fi

if [ "${sum%% *}" = "$($B cat /data.sha256)" ]; then
    echo "KICKSTART-INTEGRATION-OK"
else
    echo "KICKSTART-INTEGRATION-FAIL: /data has changed"
fi
