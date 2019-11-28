TESTS += log

LOG_SRCS  = test/log/main.c
LOG_SRCS += log.c
LOG_SRCS += eventloop.c
LOG_SRCS += ringbuffer.c

LOG_LDFLAGS += -lz
