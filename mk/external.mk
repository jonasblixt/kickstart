TOP=$(shell pwd)

# Buildroot
# https://buildroot.org/downloads/buildroot-2024.02.6.tar.gz
BR_VER = 2024.02.6
BR_SRC_URL = https://buildroot.org/downloads/buildroot-${BR_VER}.tar.gz
BR_SRC = $(TOP)/dl/buildroot-${BR_VER}
BR_SHA1 = asd

PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m

notice = $(PRINTF) "$(PASS_COLOR)$(strip $1)$(NO_COLOR)\n"

define download-n-extract
$(eval $(T)_SRC_ARCHIVE = $(TOP)/dl/$(shell basename $($(T)_SRC_URL)))
$($(T)_SRC_ARCHIVE):
	@$(PRINTF) "  GET\t$$@\n"
	$(Q)curl --progress-bar -o $$@ -L -C - "$(strip $($(T)_SRC_URL))"
	$(Q)echo "$(strip $$($(T)_SRC_SHA1))  $$@" | shasum -c
$($(T)_SRC): $($(T)_SRC_ARCHIVE)
	@$(PRINTF) "Unpacking $$@ ... "
	$(Q)tar -xf $$< -C ${TOP}/ && $(call notice, [OK])
endef

EXTERNAL_SRC = BR
$(foreach T,$(EXTERNAL_SRC),$(eval $(download-n-extract)))

