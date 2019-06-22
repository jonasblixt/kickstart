
INITRD_PKGS += pkg_popt

pkg_popt:
	@tar xf /work/dl/popt-1.16.tar.gz -C $(BUILD_DIR)/build/
	@cd $(BUILD_DIR)/build/popt-1.16 \
		&& ./autogen.sh  \
		&& ./configure CC=aarch64-linux-gnu-gcc --host arm-linux-gnu --prefix $(BUILD_DIR)/target \
		&& make  \
		&& make install
