#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>
#include <kickstart/kickstart.h>
#include <kickstart/log.h>
#include <kickstart/ringbuffer.h>

static const char *ks_log_level_str[] =
{
    "INVALID",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
};

static uint64_t log_advance_tail(struct ks_ringbuffer *rb,
                                 struct ks_ringbuffer_tail *t,
                                 uint64_t new_index)
{
    struct ks_log_entry_header *hdr = NULL;
    uint64_t current_index = t->tail_index;

    while (current_index < new_index)
    {
        hdr = (struct ks_log_entry_header *) &rb->bfr[current_index];
        current_index = (current_index + sizeof(struct ks_log_entry_header) +
            hdr->sz)%rb->bfr_sz;
    }

    return current_index;
}

static void log_in_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_source *src = io->data;
    struct ks_log *log = src->log;
    struct ks_log_entry_header hdr;
    size_t read_data;
    int rc;

    if (src->input_formatter)
    {
        read_data = read(io->fd, src->buf, KS_LOG_INPUT_BUF_SZ);
        rc = src->input_formatter(src, read_data, &hdr);

        if (rc != KS_OK)
            return;
    }
    else
    {
        /* Data is already structured */
        read_data = read(io->fd, &hdr, sizeof(hdr));

        if (read_data != sizeof(hdr))
            return;

        if (hdr.magic != KS_LOG_HEADER_MAGIC)
            return;

        read_data = read(io->fd, src->buf, KS_LOG_INPUT_BUF_SZ);

        if (read_data != hdr.sz)
            return;
    }

    ks_ringbuffer_write(log->rb, (char *) &hdr, sizeof(hdr));
    ks_ringbuffer_write(log->rb, src->buf, hdr.sz);

    /* Trigger all sinks */
    for (struct ks_log_sink *sink = log->sinks; sink; sink = sink->next)
    {
        ks_eventloop_io_oneshot(log->el, sink->io);
    }
}

static void log_out_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_sink *sink = io->data;
    struct ks_log *log = sink->log;
    struct ks_log_entry_header hdr;
    int rc;
    size_t written;
    uint16_t data_to_write;

    rc = ks_ringbuffer_read(log->rb, sink->t, (char *) &hdr,
                               sizeof(hdr));

    if (rc != KS_OK)
        return;

    ks_ringbuffer_read(log->rb, sink->t, sink->buf, hdr.sz);

    if (hdr.magic != KS_LOG_HEADER_MAGIC)
        return;

    data_to_write = hdr.sz;

    if (sink->output_formatter)
    {
        rc = sink->output_formatter(sink, hdr.sz, &data_to_write, &hdr);

        if (rc != KS_OK)
            return;
    }
    else
    {
        if (sink->write_cb)
            written = sink->write_cb(io->fd, &hdr, sizeof(hdr));
        else
            written = write(io->fd, &hdr, sizeof(hdr));

        /* TODO: Add error counters for write faliures */
        if (written != sizeof(hdr))
            return;
    }


    if (sink->write_cb)
        written = sink->write_cb(io->fd, sink->buf, data_to_write);
    else
        written = write(io->fd, sink->buf, data_to_write);

    if (written != data_to_write)
        return;

    /* Kick descriptor until buffer is empty */
    ks_eventloop_io_oneshot(log->el, sink->io);
}

int ks_log_set_input_formatter(struct ks_log_source *src,
                               ks_log_input_formatter_t formatter)
{
    if (src == NULL)
        return KS_ERR;
    if (formatter == NULL)
        return KS_ERR;

    src->input_formatter = formatter;

    return KS_OK;
}

int ks_log_set_output_formatter(struct ks_log_sink *sink,
                               ks_log_output_formatter_t formatter)
{
    if (sink == NULL)
        return KS_ERR;
    if (formatter == NULL)
        return KS_ERR;

    sink->output_formatter = formatter;

    return KS_OK;
}

const char * ks_log_level_to_string(enum ks_log_level lvl)
{
    if (lvl >= ARRAY_SIZE(ks_log_level_str))
        return ks_log_level_str[0];
    if (lvl == 0)
        return ks_log_level_str[0];

    return ks_log_level_str[lvl];
}

int ks_log_init(struct ks_log **log, struct ks_eventloop_ctx *el, size_t bfr_sz)
{

    (*log) = malloc(sizeof(struct ks_log));

    if (!(*log))
        return KS_ERR;

    memset(*log, 0, sizeof(struct ks_log));
    (*log)->el = el;

    if (ks_ringbuffer_init(&(*log)->rb, bfr_sz) != KS_OK)
    {
        return KS_ERR;
    }

    return KS_OK;
}

int ks_log_add_source(struct ks_log *log, struct ks_log_source **new_src, int fd)
{
    struct ks_eventloop_ctx *ctx = log->el;
    struct ks_eventloop_io *io;
    struct ks_log_source *src;

    if (!log)
        return KS_ERR;

    if (!ctx)
        return KS_ERR;

    if (ks_eventloop_alloc_io(ctx, &io) != KS_OK)
        return KS_ERR;

    *new_src = malloc(sizeof(struct ks_log_source));
    src = *new_src;

    if (!src)
        return KS_ERR;

    memset(src, 0, sizeof(struct ks_log_source));

    io->fd = fd;
    io->data = src;
    io->cb = log_in_cb;
    io->flags = EPOLLIN;

    src->log = log;
    src->io = io;

    if (log->sources == NULL)
    {
        log->sources = src;
    }
    else
    {
        struct ks_log_source *last = log->sources;

        while (last->next)
            last = last->next;

        last->next = src;
        src->prev = last;
    }

    return ks_eventloop_add(ctx, io);
}


int ks_log_add_sink(struct ks_log *log, struct ks_log_sink **new_sink, int fd)
{
    struct ks_eventloop_ctx *ctx = log->el;
    struct ks_eventloop_io *io;
    struct ks_log_sink *sink;
    int rc;

    *new_sink = malloc(sizeof(struct ks_log_sink));
    sink = *new_sink;

    if (!sink)
    {
        return KS_ERR;
    }

    if (ks_eventloop_alloc_io(ctx, &io) != KS_OK)
    {
        free(sink);
        return KS_ERR;
    }
    memset(sink, 0, sizeof(struct ks_log_sink));

    rc = ks_ringbuffer_new_tail(log->rb, &sink->t);

    if (rc != KS_OK)
        return rc;

    rc = ks_ringbuffer_set_tail_advance_cb(sink->t, log_advance_tail);

    if (rc != KS_OK)
    {
        free(sink->t);
        return rc;
    }

    io->fd = fd;
    io->data = sink;
    io->cb = log_out_cb;
    io->flags = EPOLLOUT | EPOLLONESHOT;

    sink->io = io;
    sink->log = log;

    if (log->sinks == NULL)
    {
        log->sinks = sink;
    }
    else
    {
        struct ks_log_sink *last = log->sinks;

        while (last->next)
            last = last->next;
        last->next = sink;
        sink->prev = last;
    }

    sink->write_cb = NULL;

    return ks_eventloop_add(ctx, io);
}

int ks_log_set_source_name(struct ks_log_source *src, const char *name)
{
    if (!src)
        return KS_ERR;
    if (!src->log)
        return KS_ERR;
    if (!name)
        return KS_ERR;

    src->name = strdup(name);
    src->source_id = (uint32_t) crc32(0L, (const Bytef *)src->name,
                                  strlen(src->name));

    return KS_OK;
}


char * ks_log_source_id_to_string(struct ks_log *log, uint32_t source_id)
{
    for (struct ks_log_source *s = log->sources; s; s = s->next)
    {
        if (source_id == s->source_id)
            return s->name;
    }

    return "Invalid";
}

int ks_log_free_source(struct ks_log_source *src)
{
    if (!src)
        return KS_ERR;

    struct ks_log *log = src->log;

    /* Remove source from evetloop */

    if (ks_eventloop_remove(log->el, src->io) != KS_OK)
        return KS_ERR;

    struct ks_log_source *p = src->prev;

    /* Remove source from log linked-list */

    if (p)
        p->next = src->next;
    else
        log->sources = src->next;

    /* Free log name */

    if (src->name)
    {
        free(src->name);
        src->name = NULL;
    }
    
    /* Free log object */
    free(src);

    return KS_OK;
}

int ks_log_free_sink(struct ks_log_sink *sink)
{
    if (!sink)
        return KS_ERR;

    struct ks_log *log = sink->log;

    if (ks_eventloop_remove(log->el, sink->io) != KS_OK)
        return KS_ERR;
    
    struct ks_log_sink *p = sink->prev;
    
    if (p)
        p->next = sink->next;
    else
        log->sinks = sink->next;

    free(sink);
    sink = NULL;

    return KS_OK;
}

int ks_log_free(struct ks_log *log)
{
    if (!log)
        return KS_ERR;

    for (struct ks_log_source *s = log->sources; s; s = s->next)
    {
        if (ks_log_free_source(s) != KS_OK)
            return KS_ERR;
    }

    for (struct ks_log_sink *s = log->sinks; s; s = s->next)
    {
        if (ks_log_free_sink(s) != KS_OK)
            return KS_ERR;
    }
    
    if (ks_ringbuffer_free(log->rb) != KS_OK)
        return KS_ERR;

    free(log);
    log = NULL;
    return KS_OK;
}
