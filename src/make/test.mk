TEST_TARGETS = 
TESTS =

TEST_BASE_SRC = 3pp/nala/nala.c
CFLAGS += -I 3pp/nala -I build-test/

-include $(wildcard test/*/makefile.mk)

%-test-target:
	$(info -------- TEST $(TEST_NAME) BEGIN --------)
	@$(BUILD_DIR)/test/test-$(TEST_NAME)

define testcase-inner
$(2)_SRCS += build-test/test/$(1)/__mocks__.c
$(2)_OBJS  = $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$(TEST_BASE_SRC))
$(2)_OBJS += $$(patsubst %.c, $$(BUILD_DIR)/%.o, $$($(2)_SRCS))

ifndef $(2)_LDFLAGS
$(2)_LDFLAGS =
endif

$(1)-test-target: TEST_NAME=$(2)
$(1)-test-link: TEST_NAME=$(2)
$(1)-test-mock: TEST_NAME=$(2)

build-test/test/$(1)/__mocks__.c: $($(2)_SRCS)
	$$(info MOCK $(1))
	@mkdir -p build-test/test/$(1)
	@$(CC) $$(CFLAGS) -E test/$(1)/*.c | \
		 nala generate_mocks
	@mv __mocks__.*  build-test/test/$(1)/	

build-test/test/test-$(2): $$($(2)_OBJS)
	$$(info LINK $(1))
	@$(CC) $$(CFLAGS) $$($(2)_OBJS) $$($(2)_LDFLAGS) \
		$$(shell cat build-test/test/$(1)/__mocks__.ld) \
		-o build-test/test/test-$(2)

TEST_TARGETS += $$(BUILD_DIR)/test/$(1)/__mocks__.c
TEST_TARGETS += $$($(2)_OBJS)
TEST_TARGETS += $$(BUILD_DIR)/test/test-$(2)
TEST_TARGETS += $(1)-test-target

endef

$(foreach testcase,$(TESTS),$(eval $(call testcase-inner,$(testcase),$(call UC,$(testcase)))))

