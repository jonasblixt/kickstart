#!/bin/sh
#
# Build the static init with musl in an Alpine container, for development
# hosts without a musl toolchain. Board builds use the Buildroot toolchain.
#
# scripts/build-alpine.sh <build dir> <bpak source dir> [cmake options...]
#
# The keystore must be the generated C code, the container has no bpak tool:
#   bpak generate keystore keystore.bpak --name kickstart > keystore.c
#
# Example:
#   scripts/build-alpine.sh build-musl ../bpak \
#       -DKICKSTART_ROOT_A=/dev/mmcblk0p5 -DKICKSTART_ROOT_B=/dev/mmcblk0p6 \
#       -DKICKSTART_KEYSTORE=/build/keystore.c
#
# The build directory is mounted at /build, paths in the options are paths in
# the container.
#
set -eu

if [ $# -lt 2 ]; then
    sed -n '3,19p' "$0"
    exit 1
fi

BUILD=$(realpath "$1")
BPAK=$(realpath "$2")
SRC=$(realpath "$(dirname "$0")/..")
shift 2

mkdir -p "$BUILD"

docker run --rm \
    -v "$SRC:/src/kickstart:ro" \
    -v "$BPAK:/src/bpak:ro" \
    -v "$BUILD:/build" \
    alpine:3.22 sh -c '
trap "chown -R '"$(id -u):$(id -g)"' /build" EXIT
set -e
apk add -q build-base cmake ninja linux-headers mbedtls-dev mbedtls-static xz-dev xz-static
cmake -S /src/kickstart -B /build/cmake -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DKICKSTART_BPAK_SOURCE_DIR=/src/bpak "$@"
cmake --build /build/cmake
strip -o /build/kickstart /build/cmake/kickstart
' sh "$@"

file "$BUILD/kickstart"
