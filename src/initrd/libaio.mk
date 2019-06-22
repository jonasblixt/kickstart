

INITRD_PKGS += pkg_libaio

pkg_libaio:
	@tar xf /work/dl/libaio-libaio-0.3.111.tar.gz -C $(BUILD_DIR)/build/
	@cd $(BUILD_DIR)/build/libaio-libaio-0.3.111 \
		&& make  CC=aarch64-linux-gnu-gcc\
		&& make prefix=$(BUILD_DIR)/target install
