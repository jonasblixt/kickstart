
# Kickstart init system
#
# Copyright (C) 2018 Jonas Persson <jonpe960@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

TARGET_BIN  = kickstart

PREFIX ?= /sbin
PKG_CONFIG ?= pkg-config
KS_SYSROOT ?= /sdk
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
STRIP=$(CROSS_COMPILE)strip
SIZE=$(CROSS_COMPILE)size
OBJCOPY=$(CROSS_COMPILE)objcopy

ifeq (,$(shell which $(PKG_CONFIG)))
$(error "No pkg-config found")
endif

ifeq (,$(shell $(PKG_CONFIG) --libs --cflags blkid))
$(error "No blkid detected")
endif

ifeq (,$(shell $(PKG_CONFIG) --libs --cflags uuid))
$(error "No blkid detected")
endif

GIT_VERSION = $(shell git describe --abbrev=4 --dirty --always --tags)

CFLAGS  += -Wall -Os
CFLAGS  += -DVERSION=\"$(GIT_VERSION)\"
CFLAGS  += $(shell $(PKG_CONFIG) --cflags blkid uuid libcryptsetup json-c blkid)
CFLAGS  += -I . --sysroot $(KS_SYSROOT)
CFLAGS  += -I $(KS_SYSROOT)/include
CFLAGS  += -MMD -MP


LIBS    = $(shell $(PKG_CONFIG) --libs blkid libcryptsetup uuid json-c blkid)
LIBS   += -ldevmapper -lpthread -lm

LDFLAGS = -L/sdk/lib

ASM_SRCS =
C_SRCS   = main.c

OBJS     += $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS))
OBJS     += $(patsubst %.S, $(BUILD_DIR)/%.o, $(ASM_SRCS))

DEPS = $(OBJS:%.o=%.d)

BUILD_DIR = build

ifdef CODE_COV
	CFLAGS += -fprofile-arcs -ftest-coverage
endif

all: $(BUILD_DIR)/$(TARGET_BIN)

$(BUILD_DIR)/$(TARGET_BIN): $(OBJS)
	@echo LINK $@ $(LDFLAGS)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -static -o $@
	@$(STRIP) --strip-all $@
$(BUILD_DIR)/%.o: %.c
	@echo CC $<
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $< -o $@

install: all
	@install -m 755 $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)

clean:
	@-rm -rf $(BUILD_DIR)/

-include $(DEPS)
