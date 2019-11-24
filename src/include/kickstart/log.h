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
 *      - Socket server for remote debugging
 *
 *  Ideas/Todos:
 *      - Translate log object name to crc32 tag
 *      - Rate limiting
 *      - Coalesce same messages in a row
 *      - Log epoch reference from boot
 *      - API call to initialize epoch
 *          - 
 *      - Timestamp is uint64
 *      - Only use malloc when allocating the main ring buffer
 *          the rest can be statically allocated
 *      - Implement ks_malloc, ks_free to support mocking of malloc/free
 *
 */

#ifndef KS_LOG_MAX_SINKS
#define KS_LOG_MAX_SINKS 8
#endif

#ifndef KS_LOG_INPUT_BUF_SZ
#define KS_LOG_INPUT_BUF_SZ 1024
#endif

#ifndef KS_LOG_OUTPUT_BUF_SZ
#define KS_LOG_OUTPUT_BUF_SZ 1024
#endif

#define KS_LOG_HEADER_MAGIC 0xd77e1ed1

enum ks_log_level
{
    KS_LOG_LEVEL_INVALID,
    KS_LOG_LEVEL_DEBUG,
    KS_LOG_LEVEL_INFO,
    KS_LOG_LEVEL_WARN,
    KS_LOG_LEVEL_ERR,
};

struct ks_log_entry_header;
struct ks_log_source;
struct ks_log_sink;

typedef int (*ks_log_input_formatter_t) (struct ks_log_source *src,
                                         uint16_t sz,
                                         struct ks_log_entry_header *hdr);

typedef int (*ks_log_output_formatter_t) (struct ks_log_sink *sink,
                                          uint16_t sz,
                                          uint16_t *new_sz,
                                          struct ks_log_entry_header *hdr);

struct ks_log
{
    struct ks_ringbuffer *rb;
    struct ks_eventloop_ctx *el;
    struct ks_log_sink *sinks;
    struct ks_log_source *sources;
};

struct ks_log_source
{
    char buf[KS_LOG_INPUT_BUF_SZ];
    char tmp[KS_LOG_INPUT_BUF_SZ];
    char *name;
    uint32_t source_id;
    ks_log_input_formatter_t input_formatter;
    struct ks_log *log;
    struct ks_eventloop_io *io;
    struct ks_log_source *next;
};

struct ks_log_sink
{
    char buf[KS_LOG_OUTPUT_BUF_SZ];
    char tmp[KS_LOG_OUTPUT_BUF_SZ];
    ks_log_output_formatter_t output_formatter;
    struct ks_ringbuffer_tail *t;
    struct ks_eventloop_io *io;
    struct ks_log *log;
    struct ks_log_sink *next;
};

struct ks_log_entry_header
{
    uint32_t magic;
    uint16_t sz;
    uint32_t source_id;
    uint64_t ts;
    enum ks_log_level lvl;
} __attribute__ ((packed));

int ks_log_init(struct ks_log *log, struct ks_eventloop_ctx *el, size_t bfr_sz);
int ks_log_add_source(struct ks_log *log, struct ks_log_source *src, int fd);
int ks_log_set_input_formatter(struct ks_log_source *src,
                               ks_log_input_formatter_t formatter);
int ks_log_add_sink(struct ks_log *log, struct ks_log_sink *sink, int fd);

int ks_log_set_output_formatter(struct ks_log_sink *sink,
                               ks_log_output_formatter_t formatter);

const char * ks_log_level_to_string(enum ks_log_level lvl);

#endif  // INCLUDE_KICKSTART_LOG_H_
