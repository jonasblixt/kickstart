
PREFIX ?= /sbin
PKG_CONFIG ?= pkg-config

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
STRIP=$(CROSS_COMPILE)strip
SIZE=$(CROSS_COMPILE)size
OBJCOPY=$(CROSS_COMPILE)objcopy

GIT_VERSION = $(shell git describe --abbrev=4 --dirty --always --tags)

CFLAGS  += -DVERSION=\"$(GIT_VERSION)\"

ifeq (,$(shell which $(PKG_CONFIG)))
$(error "No pkg-config found")
endif

define libs-inner
ifeq (,$$(shell $(PKG_CONFIG) --libs --cflags $1))
$$(error "No $1 detected")
endif

CFLAGS += $$(shell $(PKG_CONFIG) --cflags $1)
LDFLAGS += $$(shell $(PKG_CONFIG) --libs $1)
endef

libs = $(call libs-inner,$(LIBS))

UC = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')
LC = $(shell echo '$1' | tr '[:upper:]' '[:lower:]')
