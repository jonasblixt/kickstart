![KS Logo](doc/ks_logo.png)

Kickstart is the init ramdisk of [punchboot](https://github.com/jonasblixt/punchboot).
It is a small, statically linked `/init` that verifies the root file system
of the system that punchboot booted, mounts it read only with dm-verity and
hands over to its init.

```
ROM -> punchboot -> Linux -> kickstart (/init in the initramfs) -> /sbin/init of the root
           |                    |
           |  /chosen/pb,*      |  bpak header signature, dm-verity root hash
           +--------------------+
```

punchboot verifies the boot image (kernel, device tree and the initramfs with
kickstart), so the keys that kickstart is built with are covered by its
signature. Kickstart extends the chain to the root file system.

## Boot flow

1. Mount `/dev`, `/proc`, `/sys` and `/run`.
2. Read the properties punchboot adds to `/chosen` in the device tree:
   `pb,active-system`, `pb,device-uuid`, `pb,slc`, `pb,slc-active-key` and
   `pb,slc-available-keys`.
3. Read the punchboot boot state (pbstate) from the primary partition, or
   the backup when the primary is corrupt. Kickstart logs it and never
   writes it: marking a system as verified is up to the root system.
4. Select the system: `pb,active-system`, or the one enabled system in the
   pbstate if punchboot did not set it.
5. Verify the root image of that system (see below).
6. Map the file system with dm-verity, `/dev/mapper/kickstart-root`.
7. Mount it read only at `/newroot`, move `/dev`, `/proc`, `/sys` and `/run`
   into it, delete the initramfs and execute its init.

Any error restarts the system after one second. punchboot counts the failed
boot, and after the remaining boot attempts of an unverified system it rolls
back to the other one.

Started as anything but PID 1, kickstart does a dry run: steps 2 to 5 and
it prints the dm-verity table, without changing anything.

## Root images

A root image is a [bpak](https://github.com/jonasblixt/bpak) on a block
device, with a read only file system and its merkle hash tree:

| Part                | Content                                         |
| ------------------- | ----------------------------------------------- |
| `rootfs`            | squashfs, a multiple of 4 KiB                   |
| `rootfs-hash-tree`  | the hash tree, dm-verity format 1, sha256       |

The `merkle-root-hash` and `merkle-salt` meta data are in the signed header.
At boot kickstart only reads the header: it checks the signature with a key
from its keystore, the payload hash of the parts that are not marked
`dont-hash`, and the layout. dm-verity then verifies every block when it is
read, a modified block gives an I/O error, a restart or a panic
(`KICKSTART_VERITY_ON_CORRUPTION`).

The parts start at 512 byte aligned offsets in the bpak, dm-verity reads from
the start of a device, so two linear devices (`kickstart-root-data` and
`kickstart-root-hash`) map the parts and the verity device is on top of them.

```sh
mksquashfs rootfs/ rootfs.squashfs -all-root -noappend
scripts/mkroot.sh rootfs.squashfs key.pem <keystore id> <key id> root.bpak
dd if=root.bpak of=/dev/mmcblk0p3     # or punchboot part install
```

`mkroot.sh` is `bpak add --encoder merkle --set-flag dont-hash`,
`bpak set --key-id --keystore-id` and `bpak sign`.

### Keys

The keystore is compiled into kickstart, `KICKSTART_KEYSTORE` is a bpak
keystore archive, or the code from `bpak generate keystore --name kickstart`:

```sh
bpak create keystore.bpak
bpak add keystore.bpak --meta keystore-provider-id --from-string my-keystore --encoder id
bpak add keystore.bpak --part my-key --from-file my-key-pub.pem --encoder key
```

The header of a root image selects the key: its keystore id must be the
keystore of kickstart and its key id a key in it. With
`KICKSTART_REQUIRE_PB_KEYS` (default) the key must also be in
`/chosen/pb,slc-available-keys`, so a key that is revoked in punchboot is
revoked for the root images too. The key ids are the bpak ids of the key
names, use the names of punchboot's keys for the root image keys.

## Building

Kickstart is configured when it is built, it has no configuration file. The
devices are block devices, kickstart does not look up partitions.

| Option                          | Default                      |                                                 |
| ------------------------------- | ---------------------------- | ----------------------------------------------- |
| `KICKSTART_ROOT_A`, `_B`        | required                     | Block device of the root image of system A, B  |
| `KICKSTART_KEYSTORE`            | required                     | Keystore archive or its generated code          |
| `KICKSTART_PBSTATE_PRIMARY`, `_BACKUP` | not used              | pbstate block devices                           |
| `KICKSTART_ROOT_PART`           | `rootfs`                     | Part with the file system                       |
| `KICKSTART_ROOT_FSTYPE`         | `squashfs`                   |                                                 |
| `KICKSTART_ROOT_OPTIONS`        |                              | Mount options                                   |
| `KICKSTART_INIT`                | `/sbin/init`                 | Init of the root file system                    |
| `KICKSTART_CHOSEN`              | `/proc/device-tree/chosen`   |                                                 |
| `KICKSTART_DEVICE_TIMEOUT_MS`   | `10000`                      | Wait for the block devices                      |
| `KICKSTART_VERITY_ON_CORRUPTION`| `restart`                    | `eio`, `restart` or `panic`                     |
| `KICKSTART_REQUIRE_PB_KEYS`     | `ON`                         | Key must be in `pb,slc-available-keys`          |
| `KICKSTART_LOG_LEVEL`           | `info`                       | `error`, `warning`, `info` or `debug`           |
| `KICKSTART_BPAK_SOURCE_DIR`     |                              | Build libbpak from source instead of installed  |
| `KICKSTART_BPAK_TOOL`           | `bpak`                       | bpak tool of the build host                     |

`examples/jiffy.cmake` has the devices of jiffy:

```sh
cmake -B build -C examples/jiffy.cmake -DKICKSTART_KEYSTORE=keystore.bpak
cmake --build build
scripts/mkinitramfs.py --init build/kickstart -o initramfs.cpio.gz
```

The init is linked statically and the C library must be musl, configuring
with glibc or uClibc-ng fails, see [licensing](doc/licensing.md). Use a
Buildroot musl toolchain, or on a development host
`scripts/build-alpine.sh` builds it in an Alpine container.

It needs libbpak (with Mbed TLS) and the headers of Mbed TLS. The kernel
needs `CONFIG_BLK_DEV_INITRD`, `CONFIG_DEVTMPFS`, `CONFIG_BLK_DEV_DM`,
`CONFIG_DM_VERITY` and `CONFIG_SQUASHFS`. Kickstart logs to the kernel log,
which rate limits user space messages unless the kernel is booted with
`printk.devkmsg=on`. Errors are written to the console as well.

## Tests

```sh
cmake -B build-test -DKICKSTART_BUILD_INIT=OFF -DKICKSTART_BUILD_TESTS=ON \
      -DKICKSTART_SANITIZE=ON -DKICKSTART_BPAK_SOURCE_DIR=../bpak
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

The unit tests (cmocka, glibc is fine for them) use images created with the
bpak tool, mksquashfs and openssl. `test/verity_ref.py` computes the
dm-verity hash tree like the kernel, without bpak, and compares it with the
merkle tree of the images.

`test/integration/run.sh ../bpak <work dir>` boots the static init as PID 1
of a PID namespace in a privileged container on the kernel of the host: a
good image, a corrupted file system block and a bad signature. Read the
script first, it creates loop and device mapper devices on the host.

## License

BSD-3-Clause, see [LICENSE](LICENSE) and [doc/licensing.md](doc/licensing.md)
for the components in the init.
