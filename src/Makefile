# Kickstart init system
#
# Copyright (C) 2019 Jonas Blixt <jonpe960@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

KS_SYSROOT ?= /sdk

ifndef BOARD
BOARD = test
endif

BUILD_DIR = build-$(BOARD)

$(shell mkdir -p $(BUILD_DIR))
include make/common.mk

ifeq ($(BOARD),test)
include test/ll/makefile.mk
include test/ringbuffer/makefile.mk
include test/eventloop/makefile.mk
include test/log/makefile.mk
include make/test.mk
endif

LIBS =
LIBS += libnl-3.0
LIBS += libnl-route-3.0
LIBS += libnl-nf-3.0
LIBS += libnl-genl-3.0
LIBS += libnl-cli-3.0
LIBS += blkid
LIBS += libcryptsetup
LIBS += devmapper
LIBS += json-c
LIBS += uuid
LIBS += zlib

ROOTDEVICE ?= mmcblk0

ifeq ($(BOARD),test)
CFLAGS += -fprofile-arcs -ftest-coverage -g -O0
CFLAGS += -fsanitize=address
CFLAGS += -fsanitize=undefined
CFLAGS += -fsanitize=null
CFLAGS += -fsanitize=unreachable
CFLAGS += -fsanitize=signed-integer-overflow
CFLAGS += -fsanitize=alignment
CFLAGS  += -no-pie
else
CFLAGS  += -O2
$(eval $(libs))
endif

CFLAGS  += -Wall
CFLAGS  += -DROOTDEVICE=\"$(ROOTDEVICE)\"
CFLAGS  += -I $(KS_SYSROOT)/include
CFLAGS  += -I include
CFLAGS  += -MMD -MP
CFLAGS  += -I .

C_SRCS   = kickstart.c
C_SRCS  += service.c
C_SRCS  += eventloop.c
C_SRCS  += ll.c
C_SRCS  += ringbuffer.c
C_SRCS  += log.c
C_SRCS  += log_stdout_sink.c
C_SRCS  += log_utils.c
C_SRCS  += log_udp_server.c

INITRD_C_SRCS = kickstart-initrd.c

INITRD_C_SRCS  += 3pp/bearssl/sha2big.c
INITRD_C_SRCS  += 3pp/bearssl/sha2small.c
INITRD_C_SRCS  += 3pp/bearssl/dec64be.c
INITRD_C_SRCS  += 3pp/bearssl/enc64be.c
INITRD_C_SRCS  += 3pp/bearssl/dec32be.c
INITRD_C_SRCS  += 3pp/bearssl/enc32be.c
INITRD_C_SRCS  += 3pp/bearssl/hmac.c
INITRD_C_SRCS  += 3pp/bearssl/ec_prime_i31.c
INITRD_C_SRCS  += 3pp/bearssl/ec_secp256r1.c
INITRD_C_SRCS  += 3pp/bearssl/ec_secp384r1.c
INITRD_C_SRCS  += 3pp/bearssl/ec_secp521r1.c
INITRD_C_SRCS  += 3pp/bearssl/x509_decoder.c
INITRD_C_SRCS  += 3pp/bearssl/skey_decoder.c
INITRD_C_SRCS  += 3pp/bearssl/ecdsa_i31_vrfy_raw.c
INITRD_C_SRCS  += 3pp/bearssl/ecdsa_i31_vrfy_asn1.c
INITRD_C_SRCS  += 3pp/bearssl/ecdsa_i31_bits.c
INITRD_C_SRCS  += 3pp/bearssl/ecdsa_atr.c
INITRD_C_SRCS  += 3pp/bearssl/hmac_drbg.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_i62_pkcs1_vrfy.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_i62_pkcs1_sign.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_pkcs1_sig_unpad.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_pkcs1_sig_pad.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_i62_pub.c
INITRD_C_SRCS  += 3pp/bearssl/rsa_i62_priv.c
INITRD_C_SRCS  += 3pp/bearssl/i31_decode.c
INITRD_C_SRCS  += 3pp/bearssl/i31_iszero.c
INITRD_C_SRCS  += 3pp/bearssl/i31_decmod.c
INITRD_C_SRCS  += 3pp/bearssl/i31_mulacc.c
INITRD_C_SRCS  += 3pp/bearssl/i31_reduce.c
INITRD_C_SRCS  += 3pp/bearssl/i31_decred.c
INITRD_C_SRCS  += 3pp/bearssl/i31_rshift.c
INITRD_C_SRCS  += 3pp/bearssl/i31_ninv31.c
INITRD_C_SRCS  += 3pp/bearssl/i62_modpow2.c
INITRD_C_SRCS  += 3pp/bearssl/i31_encode.c
INITRD_C_SRCS  += 3pp/bearssl/i31_bitlen.c
INITRD_C_SRCS  += 3pp/bearssl/i31_modpow2.c
INITRD_C_SRCS  += 3pp/bearssl/i31_tmont.c
INITRD_C_SRCS  += 3pp/bearssl/i31_muladd.c
INITRD_C_SRCS  += 3pp/bearssl/i32_div32.c
INITRD_C_SRCS  += 3pp/bearssl/i31_sub.c
INITRD_C_SRCS  += 3pp/bearssl/i31_add.c
INITRD_C_SRCS  += 3pp/bearssl/i31_montmul.c
INITRD_C_SRCS  += 3pp/bearssl/i31_fmont.c
INITRD_C_SRCS  += 3pp/bearssl/i31_modpow.c
INITRD_C_SRCS  += 3pp/bearssl/ccopy.c
OBJS     += $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS))

INITRD_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(INITRD_C_SRCS))

DEPS = $(OBJS:%.o=%.d) $(INITRD_OBJS:%.o=%.d)

# include board/$(BOARD)/makefile.mk

.PHONY: initrd

all:  $(BUILD_DIR)/kickstart-initrd $(BUILD_DIR)/kickstart

$(BUILD_DIR)/kickstart: $(OBJS)
	@echo LINK $@
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS)  -o $@
	@$(STRIP) --strip-all $@

$(BUILD_DIR)/kickstart-initrd: $(INITRD_OBJS)
	@echo LINK $@
	@$(CC) $(CFLAGS) $(INITRD_OBJS) $(LDFLAGS) -lpthread -lm -static -o $@
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
