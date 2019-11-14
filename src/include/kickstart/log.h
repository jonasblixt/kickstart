#ifndef INCLUDE_KICKSTART_LOG_H_
#define INCLUDE_KICKSTART_LOG_H_

#include <kickstart/kickstart.h>
#include <kickstart/eventloop.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

/**
 *  - KICKSTART LOG SYSTEM -
 * 
 *
 *  FORMATTERS                 SOURCES                LOG CORE
 *  ----------                 -------                -------- 
 *
 *  +- Syslog ---------+       +- Source 0 -----+     +- Log context -+
 *  | UDP Listner      |       | Name, fd       |     | Ram buffer    |
 *  | DS /dev/log      |-------| Input buffer   |-->--| Time stamping |
 *  | Syslog formatter |       | Formatter CB   |     | Persistent    |
 *  +------------------+       +----------------+     |   storage     |
 *                                                    | Compression   |
 *                                                    |               |
 *  +- Kernel log -----+       +- Source 1 -----+     |               |
 *  | Follow /dev/kmsg |       |                |     |               |
 *  | Kernel formatter |-------|     ....       |-->--|               |
 *  |                  |       |                |     |               |
 *  +------------------+       +----------------+     |               |
 *                                                    |               |
 *                                                    |               |
 *  Eventloop handle ---------------------------------|               |
 *  API ----------------------<>----------------------|               |
 *                                                    |               |
 *                                                    |               |
 *                                                    |               |
 *  +-- Sink 0 -------+                               |               |
 *  | fd              |                               |               |
 *  | Output buffer   |----------<--------------------|               |
 *  |                 |                               |               |
 *  +-----------------+                               |               |
 *         .                                          |               |
 *         .                                          |               |
 *         .                                          |               |
 *         .                                          |               |
 *       sink n-1                                     |               |
 *                                                    |               |
 *                                                    |               |
 *                                                    +---------------+
 *
 *
 * 
 *
 *  Sink/Source details:
 * 
 *  Each source has the following properties:
 *      - Collect incoming data on a file descriptor
 *      - Write structured data into the main ring buffer
 *      - If source data is not already formatted correctly an optional 
 *          formatter callback must be implemented
 *      - Trigger sinks whenever a new log message has been posted on the
 *          ring buffer
 *
 *  Sinks have the following properties:
 *      - Maintains an output buffer that should hold one output message
 *      - File descriptor
 *
 *
 *  Notes: 
 *
 *  Possible sources:
 *      - Kernel log, /dev/kmsg
 *      - Stdout/Stderr from sub processes
 *      - Remote log targets, e.g. VM guest, co-processor
 *      - Internal sources
 *      - Syslog UDP listner
 *      - Syslog domain-socket listner
 *
 *  Possible sinks:
 *      - Stdout
 *      - Remote debugger
 *
 *  Ideas:
 *      - Translate log object name to crc32 tag
 *      - Rate limiting
 *      - Coalesce same messages in a row
 *      - Log epoch reference from boot
 *          o 
 *      - Timestamp is uint64
 *      
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
    char buf[1024];
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
