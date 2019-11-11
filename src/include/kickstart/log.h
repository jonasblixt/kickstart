#ifndef INCLUDE_KICKSTART_LOG_H_
#define INCLUDE_KICKSTART_LOG_H_

#include <kickstart/kickstart.h>
#include <kickstart/eventloop.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

/**
 *
 * Log core
 *
 *                          +- Log object 0 -+     +- Log context -+
 *  Source 0   --- (Info) --| Name           |     | Ram buffer    |
 *  Source 1   --- (Warn) --|                |-----| Time stamping |
 *  Source n-1 --- (Err) ---|                |     | Persistent    |
 *                          +----------------+     |   storage     |
 *                                                 | Compression   |
 *                                                 |               |
 *                          +- Log object 1 -+     |               |
 *  Source 0   --- (Info) --| Name           |     |               |
 *  Source 1   --- (Warn) --|                |-----|               |
 *  Source n-1 --- (Err) ---|                |     |               |
 *                          +----------------+     +---------------+
 *  Eventloop handle --------------------------------^   |
 *                                                       |
 *  Sink 0   <---+---------------------------------------+
 *  Sink 1   <---+
 *  Sink n-1 <---+
 *
 *  Possible sources:
 *      - Kernel log, /dev/kmsg
 *      - Stdout/Stderr from sub processes
 *      - Remote log targets, e.g. VM guest, co-processor
 *      - Internal sources
 *
 *  Possible sinks:
 *      - Stdout
 *      - Remote debugger
 *
 *  Ideas:
 *      - Translate log object name to crc32 tag
 *
 *
 */

#ifndef KS_LOG_MAX_SINKS
#define KS_LOG_MAX_SINKS 8
#endif

struct ks_log_ctx;

struct ks_log_object
{
    char *name;
    struct ks_log_ctx *log_ctx;
    struct ks_log_source *sources;
    struct ks_log_object *next;
};

struct ks_log_ctx
{
    struct ks_ringbuffer *rb;
    struct ks_eventloop_ctx *el;
    struct ks_log_sink *sinks;
    struct ks_log_object *objects;
};

enum ks_log_level
{
    KS_LOG_LEVEL_INFO,
    KS_LOG_LEVEL_WARN,
    KS_LOG_LEVEL_ERR,
};

struct ks_log_source
{
    char *input_buf;
    size_t input_buf_sz;
    uint32_t input_buf_off;
    enum ks_log_level lvl;
    struct ks_log_object *log_object;
    struct ks_log_source *next;
};

struct ks_log_sink
{
    struct ks_ringbuffer_tail *t;
    struct ks_eventloop_io *io;
    struct ks_log_ctx *log_ctx;
    struct ks_log_sink *next;
};


int ks_log_init(struct ks_eventloop_ctx *el,
                struct ks_log_ctx **ctx,
                size_t bfr_sz);

int ks_log_create(struct ks_log_ctx *ctx,
                  struct ks_log_object **obj,
                  const char *name);

int ks_log_add_source(struct ks_log_object *obj,
                      enum ks_log_level lvl,
                      size_t buf_sz,
                      int fd);

int ks_log_add_sink(struct ks_log_ctx *log, int fd);

#endif  // INCLUDE_KICKSTART_LOG_H_
