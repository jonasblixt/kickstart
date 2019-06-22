
ARCH = armv8a
PLAT_IMAGE = ks_plat_imx8x:latest

plat_step:
	@$(DOCKER) build -t $(PLAT_IMAGE) --build-arg PARENT_IMAGE=$(ARCH_IMAGE) \
	   			-f plat/imx8x/plat.Dockerfile .
