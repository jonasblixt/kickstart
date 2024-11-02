TOP=$(shell pwd)
BOARD ?= qemu
BR_EXTERNAL := $(shell readlink -f $(TOP)/ks-external)
BR_BUILD := $(shell readlink -f $(TOP)/build-$(BOARD))
BR_ARGS ?=

all: help

.PHONY: help

help:
	@echo HELP $(BR_EXTERNAL)

image: dl
	@make -C $(TOP)/buildroot-$(BR_VER) \
		O=$(BR_BUILD) \
		BR2_EXTERNAL=$(BR_EXTERNAL) \
		$(BR_ARGS)

br-%: dl
	@make -C $(TOP)/buildroot-$(BR_VER) \
		O=$(BR_BUILD) \
		BR2_EXTERNAL=$(BR_EXTERNAL) \
		$(patsubst br-%,%,$(@)) \
		$(BR_ARGS)

dl: $(BR_SRC)

include mk/external.mk
include mk/qemu.mk
