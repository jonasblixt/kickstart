TEST_TARGETS = 
TESTS =

TEST_BASE_SRC = 3pp/narwhal/narwhal.c
CFLAGS += -I 3pp/narwhal

-include $(wildcard test/*/makefile.mk)

%-test-target:
	$(info -------- TEST $(TEST_NAME) BEGIN --------)
	@$(BUILD_DIR)/test-$(TEST_NAME)
%-test-mock:
	$(info MOCK $(call LC,$(TEST_NAME)))
	@$(CC) $(CFLAGS) -E test/$(call LC,$(TEST_NAME))/*.c | \
		narmock -g -d test/$(call LC,$(TEST_NAME))/
%-test-link:
	@$(CC) $(CFLAGS) $($(TEST_NAME)_OBJS) $($(2)_LDFLAGS) \
		$(shell narmock -f -d test/$(call LC,$(TEST_NAME))/) \
		-o $(BUILD_DIR)/test-$(TEST_NAME)

define testcase-inner
$(2)_SRCS += test/$(1)/__mocks__.c

$(2)_OBJS  = $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$(TEST_BASE_SRC))
$(2)_OBJS += $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$($(2)_SRCS))

ifndef $(2)_LDFLAGS
$(2)_LDFLAGS =
endif

$(1)-test-target: TEST_NAME=$(2)
$(1)-test-link: TEST_NAME=$(2)
$(1)-test-mock: TEST_NAME=$(2)

TEST_TARGETS += $(1)-test-mock
TEST_TARGETS += $$($(2)_OBJS)
TEST_TARGETS += $(1)-test-link
TEST_TARGETS += $(1)-test-target

endef

$(foreach testcase,$(TESTS),$(eval $(call testcase-inner,$(testcase),$(call UC,$(testcase)))))

