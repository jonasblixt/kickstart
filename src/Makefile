
DOCKER ?= $(shell which docker)
BUILD_DIR = $(shell readlink -f .)/build
PKG_CONFIG_PATH=$(BUILD_DIR)/target/lib/pkgconfig

include target/$(TARGET)/makefile.mk
include plat/$(PLAT)/makefile.mk
include arch/$(ARCH)/makefile.mk

include initrd/makefile.mk

all: arch_step plat_step initrd_step target_step
	@echo Building

target_step:
	@echo target

.DEFAULT_GOAL := all

