KS_BUILD_HOOKS += ks_initrd_create


ks_initrd_create:
	@echo Creating initrd...
	@bash -c "mkdir -p /tmp/initrd/proc \
    && mkdir -p /tmp/initrd/sys \
    && mkdir -p /tmp/initrd/dev \
    && mkdir -p /tmp/initrd/tmp \
    && mkdir -p /tmp/initrd/newroot \
    && cp /pkg/pki/* /tmp/initrd/ \
    && cp /ksbuild/build/kickstart-initrd /tmp/initrd/init \
    && cp /ksbuild/build/roothash /tmp/initrd/roothash \
    && cd /tmp/initrd \
    && find . | cpio -H newc -o > /pkg/build/initrd.cpio"
