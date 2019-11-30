# Kickstart init system
#
# Copyright (C) 2019 Jonas Blixt <jonpe960@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

TARGET_BIN  = kickstart

KS_SYSROOT ?= /sdk

ifndef BOARD
BOARD = test
endif

BUILD_DIR = build-$(BOARD)

$(shell mkdir -p $(BUILD_DIR))
include make/common.mk
include make/test.mk

LIBS  = blkid
LIBS += uuid
LIBS += libnl-3.0
LIBS += json-c
LIBS += libnl-route-3.0
LIBS += libnl-nf-3.0
LIBS += zlib

# $(eval $(libs))

ROOTDEVICE ?= mmcblk0


ifeq ($(BOARD),test)
CFLAGS  += -fprofile-arcs -ftest-coverage -g -O0
CFLAGS += -fsanitize=address
else
CFLAGS  += -O2
endif

CFLAGS  += -Wall
CFLAGS  += -no-pie
CFLAGS  += -DROOTDEVICE=\"$(ROOTDEVICE)\"
CFLAGS  += -I $(KS_SYSROOT)/include
CFLAGS  += -I include
CFLAGS  += -MMD -MP

ASM_SRCS =
C_SRCS   = kickstart.c
C_SRCS  += service.c

OBJS     += $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS))
OBJS     += $(patsubst %.S, $(BUILD_DIR)/%.o, $(ASM_SRCS))

DEPS = $(OBJS:%.o=%.d)


include board/$(BOARD)/makefile.mk

.PHONY: initrd

all: $(BUILD_DIR)/$(TARGET_BIN)

$(BUILD_DIR)/$(TARGET_BIN): $(OBJS)
	@echo LINK $@
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS)  -o $@
	@$(STRIP) --strip-all $@

$(BUILD_DIR)/%.o: %.c
	@echo CC $<
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $< -o $@

install: all
	@install -m 755 $(BUILD_DIR)/$(TARGET_BIN) $(PREFIX)

clean:
	@-rm -rf build/
	@-rm -rf build-*/

test: $(TEST_TARGETS)

gcovr:
	@gcovr --exclude test --exclude build-test --exclude 3pp

-include $(DEPS)
