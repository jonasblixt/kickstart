TEST_TARGETS = 
TESTS =

TEST_BASE_SRC = 3pp/narwhal.c
CFLAGS  += -I 3pp/narwhal

-include $(wildcard test/*/makefile.mk)

%-test-target:
	$(info -------- TEST $(TEST_NAME) BEGIN --------)
	@$(BUILD_DIR)/test-$(TEST_NAME)

%-test-link:
	@$(CC) $($(2)_LDFLAGS) $($(TEST_NAME)_OBJS) \
				-o $(BUILD_DIR)/test-$(TEST_NAME)

define testcase-inner
$(2)_OBJS  = $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$(TEST_BASE_SRC))
$(2)_OBJS += $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$($(2)_SRCS))

ifndef $(2)_LDFLAGS
$(2)_LDFLAGS =
endif

$(1)-test-target: TEST_NAME=$(2)
$(1)-test-link: TEST_NAME=$(2)

TEST_TARGETS += $$($(2)_OBJS)
TEST_TARGETS += $(1)-test-link
TEST_TARGETS += $(1)-test-target

endef

$(foreach testcase,$(TESTS),$(eval $(call testcase-inner,\
							$(testcase),$(call UC,$(testcase)))))

