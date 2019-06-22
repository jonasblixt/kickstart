
INITRD_PKGS += pkg_lvm2

pkg_lvm2:
	@tar xf /work/dl/LVM2.2.03.05.tgz -C $(BUILD_DIR)/build/
	@cd $(BUILD_DIR)/build/LVM2.2.03.05 && \
	ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes \
			./configure --host aarch64-linux-gnu --prefix $(BUILD_DIR)/target \
			CFLAGS=-I/work/src/build/target/include \
			LDFLAGS=-L/work/src/build/target/lib \
			--disable-nls \
			--disable-cmdlib \
			--disable-makeinstall-chown \
			--disable-dmeventd \
			--disable-fsadm \
			--disable-readline \
			--disable-selinux \
		&& make \
		&& make install
	@cp $(BUILD_DIR)/target/lib/libdevmapper.so.1.02 $(BUILD_DIR)/initrd/lib
