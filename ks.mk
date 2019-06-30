GIT_VERSION = $(shell git describe --abbrev=4 --dirty --always --tags)

KS_PKG = kickstart
KS_PKG_VERSION = $(GIT_VERSION)

KS_SDK_ARTEFACTS += build/kickstart-initrd:/sdk/initrd/init
KS_SDK_ARTEFACTS += build/create_rootfs.sh:/sdk

kickstart_build:
	@echo kickstart_build
	@make -C /pkg/src/initrd CROSS_COMPILE=aarch64-linux-gnu-
	@cp /pkg/src/build/kickstart-initrd /ksbuild/build
	@cp /pkg/src/initrd/40-rootfs.mk /ksbuild/build
	@cp /pkg/src/initrd/50-initrd.mk /ksbuild/build
	@cp /pkg/src/create_rootfs.sh /ksbuild/build

kickstart_clean:
	@make -C /pkg/src/initrd CROSS_COMPILE=aarch64-linux-gnu- clean
