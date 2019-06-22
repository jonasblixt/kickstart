
INITRD_PKGS += pkg_cryptosetup


pkg_cryptosetup:
	@tar xf /work/dl/cryptsetup-2.1.0.tar.xz -C $(BUILD_DIR)/build/
	@cd $(BUILD_DIR)/build/cryptsetup-2.1.0 \
		&&  ./configure --host aarch64-linux-gnu --prefix $(BUILD_DIR)/target \
			CFLAGS=-I/work/src/build/target/include \
			LDFLAGS=-L/work/src/build/target/lib \
			PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
			--with-crypto_backend=kernel \
		&& make  \
		&& make install
	@cp $(BUILD_DIR)/target/lib/libcryptsetup.so.12 $(BUILD_DIR)/initrd/lib
