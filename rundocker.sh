#!/bin/sh

docker run -it -u $(id -u $USER) --rm \
    -v $(readlink -f .):/work \
    -v $(readlink -f .)/src/build:/output \
    -v $(readlink -f .)/src/rootfs/:/input \
    ks-builder:latest bash -c "make CROSS_COMPILE=aarch64-linux-gnu- -C /work/src && cp /work/src/build/kickstart /work/src/rootfs/ && /create_rootfs "

