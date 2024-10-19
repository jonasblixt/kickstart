TOP=$(shell pwd)
BOARD ?= qemu
BR_EXTERNAL := $(shell readlink -f $(TOP)/ks-external)
BR_BUILD := $(shell readlink -f $(TOP)/build-$(BOARD))
BR_ARGS ?=

-include mk/external.mk

all: help

.PHONY: help

help:
	@echo HELP $(BR_EXTERNAL)

image: dl
	@make -C $(TOP)/buildroot-$(BR_VER) O=$(BR_BUILD) BR2_EXTERNAL=$(BR_EXTERNAL) $(BR_ARGS)

br-%: dl
	@make -C $(TOP)/buildroot-$(BR_VER) O=$(BR_BUILD) BR2_EXTERNAL=$(BR_EXTERNAL) $(patsubst br-%,%,$(@)) $(BR_ARGS)

QEMU ?= qemu-system-aarch64
QEMU_AUDIO_DRV = "none"
QEMU_FLAGS  = -machine virt,secure=on -m 1G -smp 2
QEMU_FLAGS += -nographic
QEMU_FLAGS += -cpu cortex-a57
# ATF bl1 boot loader
QEMU_FLAGS += -bios $(BR_BUILD)/images/bl1.bin
# Virtio rootfs
QEMU_FLAGS += -drive file=$(BR_BUILD)/images/rootfs.squashfs,if=none,format=raw,id=hd0
QEMU_FLAGS += -device virtio-blk-device,drive=hd0
# Kernel image
QEMU_FLAGS += -kernel $(BR_BUILD)/images/Image -no-acpi
# Disable default NIC
QEMU_FLAGS += -net none
# Kernel boot parameters
QEMU_FLAGS += -append "rootwait root=/dev/vda console=ttyAMA0 keep_bootcon"
# Enable logging of unimplemeted features
QEMU_FLAGS += -d unimp
# Enable semihosting, only bl1 and kernel is loaded through qemu
# bl2, bl31 and optee are loaded through the semihosting interface.
QEMU_FLAGS += -semihosting-config enable=on,target=native

# Note: ATF expects other images to be available in the directory where qemu
# is started.
.PHONY: qemu
qemu:
	@cd $(BR_BUILD)/images && $(QEMU) $(QEMU_FLAGS) $(QEMU_AUX_FLAGS)

dl: $(BR_SRC)

