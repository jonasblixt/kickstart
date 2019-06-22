
ARCH_IMAGE = ks_arch_armv8:latest

arch_step:
	@$(DOCKER) build -t $(ARCH_IMAGE) -f arch/armv8a/arch.Dockerfile .
