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
QEMU_FLAGS  = -machine virt -m 1G
QEMU_FLAGS += -nographic
QEMU_FLAGS += -cpu cortex-a57
# Virtio rootfs
QEMU_FLAGS += -drive file=$(BR_BUILD)/images/rootfs.squashfs,if=none,format=raw,id=hd0
QEMU_FLAGS += -device virtio-blk-device,drive=hd0
# Kernel image
QEMU_FLAGS += -kernel $(BR_BUILD)/images/Image
# Disable default NIC
QEMU_FLAGS += -net none
# Kernel boot parameters
QEMU_FLAGS += -append "rootwait root=/dev/vda console=ttyAMA0"

.PHONY: qemu
qemu:
	@$(QEMU) $(QEMU_FLAGS) $(QEMU_AUX_FLAGS)

dl: $(BR_SRC)

