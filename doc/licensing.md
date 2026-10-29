# Licensing of the init

The init is one statically linked binary. Everything in it must allow static
linking without obligations beyond attribution, so that kickstart can be
shipped in any product:

| Component    | License                                   | In the init                                |
| ------------ | ----------------------------------------- | ------------------------------------------ |
| kickstart    | BSD-3-Clause                              | all of it                                  |
| libbpak      | BSD-3-Clause                              | header, verify, merkle, keystore, crypto   |
| Mbed TLS     | Apache-2.0 (dual Apache-2.0 / GPL-2.0-or-later, Apache-2.0 is used) | libmbedcrypto: sha2, ecdsa, rsa, pk |
| musl         | MIT                                       | the C library                              |
| libgcc       | GPL-3.0 with the GCC Runtime Library Exception | compiler support routines             |

The libraries are linked by name and the linker only pulls in the objects
that are used. libbpak's vendored heatshrink and sais and liblzma are not in
the init (no `lzma_`, `heatshrink_` or `sais` symbols, check with `nm` on the
unstripped `build/kickstart`).

Not allowed in the init:

- glibc and uClibc-ng, LGPL-2.1. Static linking requires that users can
  relink the binary with a modified library. CMake refuses to build the init
  with them.
- libcryptsetup, libdevmapper and libblkid, LGPL. Kickstart sets up the
  device mapper with the ioctls of `<linux/dm-ioctl.h>` instead.

When the init is distributed, include the BSD-3-Clause notices of kickstart
and bpak, the Apache-2.0 license of Mbed TLS (and its NOTICE, if the version
has one) and the MIT notice of musl. The unit tests are not distributed and
use the host's glibc and cmocka (Apache-2.0).
