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
	@make -j$(nproc) -C $(TOP)/buildroot-$(BR_VER) O=$(BR_BUILD) BR2_EXTERNAL=$(BR_EXTERNAL) $(BR_ARGS)

br-%: dl
	@make -j$(nproc) -C $(TOP)/buildroot-$(BR_VER) O=$(BR_BUILD) BR2_EXTERNAL=$(BR_EXTERNAL) $(patsubst br-%,%,$(@)) $(BR_ARGS)

dl: $(BR_SRC)

