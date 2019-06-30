KS_BUILD_HOOKS += ks_rootfs_create


ks_rootfs_create:
	@echo Creating rootfs...
	@/sdk/create_rootfs.sh
