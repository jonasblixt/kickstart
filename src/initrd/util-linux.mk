
PKG_UTIL_LINUX = util-linux-2.34

INITRD_PKGS += pkg_util_linux

pkg_util_linux:
	@tar xf /work/dl/$(PKG_UTIL_LINUX).tar.gz -C $(BUILD_DIR)/build/
	@cd $(BUILD_DIR)/build/$(PKG_UTIL_LINUX) \
		&& ./autogen.sh  \
		&& ./configure --host aarch64-linux-gnu --prefix $(BUILD_DIR)/target \
		&& make  \
		&& make install
	@cp $(BUILD_DIR)/build/target/lib/libblkid.so.1 $(BUILD_DIR)/initrd/lib
	@cp $(BUILD_DIR)/build/target/lib/libuuid.so.1 $(BUILD_DIR)/initrd/lib
