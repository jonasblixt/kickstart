
include initrd/util-linux.mk
include initrd/popt.mk
include initrd/libaio.mk
include initrd/lvm2.mk
include initrd/jsonc.mk
include initrd/cryptosetup.mk

initrd_packages: $(INITRD_PKGS)

initrd_step:
	@echo Building initrd
	@mkdir -p $(BUILD_DIR)/initrd/proc
	@mkdir -p $(BUILD_DIR)/initrd/dev
	@mkdir -p $(BUILD_DIR)/initrd/lib
	@mkdir -p $(BUILD_DIR)/initrd/sys
	@mkdir -p $(BUILD_DIR)/build
	@mkdir -p $(BUILD_DIR)/target
	@$(DOCKER) run -v $(shell readlink -f ../):/work $(PLAT_IMAGE) \
		make -C /work/src TARGET=$(TARGET) initrd_packages

initrd_cpio:
	@make -C kickstart/ CROSS_COMPILE=aarch64-linux-gnu- clean
	@make -C kickstart/ CROSS_COMPILE=aarch64-linux-gnu-
	@mkdir -p $(BUILD_DIR)/initrd/newroot
	@cp /usr/aarch64-linux-gnu/lib/libc.so.6 $(BUILD_DIR)/initrd/lib
	@cp /usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1 $(BUILD_DIR)/initrd/lib
	@cp /usr/aarch64-linux-gnu/lib/libpthread.so.0 $(BUILD_DIR)/initrd/lib
	@cp /usr/aarch64-linux-gnu/lib/libm.so.6 $(BUILD_DIR)/initrd/lib
	@cp kickstart/build/kickstart $(BUILD_DIR)/initrd/init
	@cd $(BUILD_DIR)/initrd && \
		 find . | cpio -H newc -o > ../initramfs.cpio
