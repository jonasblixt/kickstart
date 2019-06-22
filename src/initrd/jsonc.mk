

INITRD_PKGS += pkg_jsonc

pkg_jsonc:
	@tar xf /work/dl/json-c-0.13.1-20180305.tar.gz -C $(BUILD_DIR)/build/
	PKG_CONFIG_PATH=/work/src/build/target/lib/pkgconfig \
		&& cd $(BUILD_DIR)/build/json-c-json-c-0.13.1-20180305 \
		&& ./configure --host aarch64-linux-gnu --prefix $(BUILD_DIR)/target \
			CFLAGS=-I/work/src/build/target/include \
			LDFLAGS=-L/work/src/build/target/lib \
		&& make  \
		&& make install
	@cp $(BUILD_DIR)/target/lib/libjson-c.so.4 $(BUILD_DIR)/initrd/lib
