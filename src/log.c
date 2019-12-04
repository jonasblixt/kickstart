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
        {
            src->stats.formatter_err++;
            return;
        }
    }
    else
    {

        /* Data is already structured */
        read_data = read(io->fd, &hdr, sizeof(hdr));

        if (read_data != sizeof(hdr))
        {
            src->stats.header_err++;
            return;
        }

        if (hdr.magic != KS_LOG_HEADER_MAGIC)
        {
            src->stats.header_err++;
            return;
        }

        read_data = read(io->fd, src->buf, KS_LOG_INPUT_BUF_SZ);

        if (read_data != hdr.sz)
        {
            src->stats.data_err++;
            return;
        }
    }

    if (ks_ringbuffer_write(log->rb, (char *) &hdr, sizeof(hdr)) != KS_OK)
    {
        src->stats.data_err++;
        return;
    }

    if (ks_ringbuffer_write(log->rb, src->buf, hdr.sz) != KS_OK)
    {
        src->stats.data_err++;
        return;
    }

    /* Trigger all sinks */
    ks_ll_foreach(log->sinks, s)
    {
        struct ks_log_sink *sink = (struct ks_log_sink *) s->data;
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
    {
        sink->stats.header_err++;
        return;
    }

    ks_ringbuffer_read(log->rb, sink->t, sink->buf, hdr.sz);

    if (hdr.magic != KS_LOG_HEADER_MAGIC)
    {
        sink->stats.header_err++;
        return;
    }

    data_to_write = hdr.sz;

    if (sink->output_formatter)
    {
        rc = sink->output_formatter(sink, hdr.sz, &data_to_write, &hdr);

        if (rc != KS_OK)
        {
            sink->stats.formatter_err++;
            return;
        }
    }
    else
    {
        if (sink->write_cb)
            written = sink->write_cb(io->fd, &hdr, sizeof(hdr));
        else
            written = write(io->fd, &hdr, sizeof(hdr));

        if (written != sizeof(hdr))
        {
            sink->stats.data_err++;
            return;
        }
    }


    if (sink->write_cb)
        written = sink->write_cb(io->fd, sink->buf, data_to_write);
    else
        written = write(io->fd, sink->buf, data_to_write);

    if (written != data_to_write)
    {
        sink->stats.data_err++;
        return;
    }

    /* Kick descriptor until buffer is empty */
    ks_eventloop_io_oneshot(log->el, sink->io);
}

int ks_log_set_input_formatter(struct ks_log_source *src,
                               ks_log_input_formatter_t formatter)
{
    if (!src)
        return KS_ERR;
    if (!formatter)
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

int ks_log_init(struct ks_log **new_log, struct ks_eventloop_ctx *el,
                size_t bfr_sz)
{
    int rc;
    struct ks_log *log;

    if (!new_log)
        return KS_ERR;
    if (!el)
        return KS_ERR;
    if (!bfr_sz)
        return KS_ERR;

    log = malloc(sizeof(struct ks_log));

    if (!log)
        return KS_ERR;

    memset(log, 0, sizeof(struct ks_log));
    log->el = el;

    if (ks_ringbuffer_init(&log->rb, bfr_sz) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_log;
    }

    if (ks_ll_init(&log->sources) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_rb;
    }

    if (ks_ll_init(&log->sinks) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_sources;
    }

    *new_log = log;
    return KS_OK;

err_free_sources:
    ks_ll_free(&log->sources);
err_free_rb:
    ks_ringbuffer_free(log->rb);
err_free_log:
    free(log);
    *new_log = NULL;
    return rc;
}

int ks_log_add_source(struct ks_log *log, struct ks_log_source **new_src, int fd)
{
    struct ks_eventloop_ctx *ctx;
    struct ks_eventloop_io *io;
    struct ks_log_source *src;
    int rc;

    if (!log)
        return KS_ERR;

    ctx = log->el;

    if (!ctx)
        return KS_ERR;

    if (ks_eventloop_alloc_io(ctx, &io) != KS_OK)
        return KS_ERR;

    if (ks_ll_append2(log->sources, (void **) &src,
                sizeof(struct ks_log_source)) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_eventloop_io;
    }

    io->fd = fd;
    io->data = src;
    io->cb = log_in_cb;
    io->flags = EPOLLIN;

    src->log = log;
    src->io = io;

    *new_src = src;

    return ks_eventloop_add(ctx, io);

err_free_eventloop_io:
    ks_eventloop_remove(ctx, io);
    return rc;
}


int ks_log_add_sink(struct ks_log *log, struct ks_log_sink **new_sink, int fd)
{
    struct ks_eventloop_io *io;
    int rc;

    if (!log)
        return KS_ERR;
    if (!new_sink)
        return KS_ERR;
    if (!fd)
        return KS_ERR;

    struct ks_eventloop_ctx *ctx = log->el;

    if (ks_eventloop_alloc_io(ctx, &io) != KS_OK)
        return KS_ERR;

    struct ks_log_sink *sink;

    if (ks_ll_append2(log->sinks, (void **) &sink,
                        sizeof(struct ks_log_sink)) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_eventloop_io;
    }

    *new_sink = sink;

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

    return ks_eventloop_add(ctx, io);

err_free_eventloop_io:
    ks_eventloop_remove(ctx, io);
    return rc;
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
    if (source_id == 0)
        return "KS";

    ks_ll_foreach(log->sources, item)
    {
        struct ks_log_source *s = (struct ks_log_source *) item->data;
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

    if (!log->sources)
        return KS_ERR;

    /* Remove source from evetloop */

    if (ks_eventloop_remove(log->el, src->io) != KS_OK)
        return KS_ERR;

    /* Free log name */

    if (src->name)
    {
        free(src->name);
        src->name = NULL;
    }

    struct ks_ll_item *item;
    ks_ll_data2item(log->sources, src, &item);
    return ks_ll_remove(log->sources, item);
}

int ks_log_free_sink(struct ks_log_sink *sink)
{
    if (!sink)
        return KS_ERR;

    struct ks_log *log = sink->log;

    if (!log)
        return KS_ERR;

    if (ks_eventloop_remove(log->el, sink->io) != KS_OK)
        return KS_ERR;

    struct ks_ll_item *item;
    ks_ll_data2item(log->sinks, sink, &item);

    return ks_ll_remove(log->sinks, item);
}

int ks_log_free(struct ks_log **log)
{
    if (!log)
        return KS_ERR;

    if (!(*log))
        return KS_ERR;

    if (ks_ll_free(&(*log)->sources) != KS_OK)
        return KS_ERR;
    
    if (ks_ll_free(&(*log)->sinks) != KS_OK)
        return KS_ERR;

    if (ks_ringbuffer_free((*log)->rb) != KS_OK)
        return KS_ERR;

    free(*log);
    *log = NULL;
    return KS_OK;
}
