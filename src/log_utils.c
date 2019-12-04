#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <kickstart/kickstart.h>
#include <kickstart/log.h>

static char internal_buffer[KS_LOG_INPUT_BUF_SZ];

int ks_log_printf(struct ks_log *log, enum ks_log_level lvl,
                  const char *fmt, ...)
{
    va_list args;
    struct ks_log_entry_header hdr;

    va_start(args, fmt);
    vsnprintf(internal_buffer, KS_LOG_INPUT_BUF_SZ, fmt, args);
    va_end(args);

    hdr.magic = KS_LOG_HEADER_MAGIC;
    hdr.sz = strlen(internal_buffer);
    hdr.source_id = 0; /* Internal kickstart source */
    clock_gettime(CLOCK_MONOTONIC, &hdr.ts);
    hdr.lvl = lvl;


    if (ks_ringbuffer_write(log->rb, (char *) &hdr, sizeof(hdr)) != KS_OK)
        return;

    if (ks_ringbuffer_write(log->rb, internal_buffer, hdr.sz) != KS_OK)
        return;

    /* Trigger all sinks */
    ks_ll_foreach(log->sinks, s)
    {
        struct ks_log_sink *sink = (struct ks_log_sink *) s->data;
        ks_eventloop_io_oneshot(log->el, sink->io);
    }
}
