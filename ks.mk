GIT_VERSION = $(shell git describe --abbrev=4 --dirty --always --tags)

KS_PKG = kickstart
KS_PKG_VERSION = $(GIT_VERSION)

kickstart_build:
	@echo kickstart_build
	@make -C /pkg/src/initrd CROSS_COMPILE=aarch64-linux-gnu- clean
	@make -C /pkg/src/initrd CROSS_COMPILE=aarch64-linux-gnu-

kickstart_sdk_populate:
	cp -a /pkg/src/build
