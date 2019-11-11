#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <kickstart/log.h>
#include <kickstart/ringbuffer.h>

const char *ks_log_level_str[] =
{
    "INFO   ",
    "WARNING",
    "ERROR  ",
};

static void __ks_log_in_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_source *src = io->data;
    struct ks_log_object *obj = src->log_object;
    struct ks_log_ctx *ctx = obj->log_ctx;

    size_t read_data = read(io->fd,
                            &src->input_buf[src->input_buf_off],
                            src->input_buf_sz);

find_next_line:
    for (int32_t i = 0; i < read_data; i++)
    {
        char c = src->input_buf[i+src->input_buf_off];
        
        if (c == '\r')
            src->input_buf[i+src->input_buf_off] = ' ';

        if (c == '\n')
        {
            ks_ringbuffer_write(ctx->rb, &i, 4);
            ks_ringbuffer_write(ctx->rb,
                                &src->input_buf[i+src->input_buf_off],
                                i);
            src->input_buf_off += i;
            goto find_next_line;
        }
    }

    src->input_buf_off = 0;

    /* Trigger all sinks */
    for (struct ks_log_sink *sink = ctx->sinks; sink; sink = sink->next)
    {
        ks_eventloop_io_oneshot(ctx->el, sink->io);
    }
}

static void __ks_log_out_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_sink *sink = io->data;
    struct ks_log_ctx *ctx = sink->log_ctx;

    int32_t log_entry_sz = 0;

    ks_ringbuffer_read(ctx->rb, sink->t, &log_entry_sz, 4);

    if (log_entry_sz)
    {
        
    }
}

int ks_log_init(struct ks_eventloop_ctx *el,
                struct ks_log_ctx **ctx,
                size_t bfr_sz)
{
    struct ks_log_ctx *log_ctx = NULL;

    if (ctx == NULL)
        return KS_ERR;

    log_ctx = malloc(sizeof(struct ks_log_ctx));

    if (log_ctx == NULL)
        return KS_ERR;

    memset(log_ctx, 0, sizeof(struct ks_log_ctx));
    log_ctx->el = el;

    if (ks_ringbuffer_init(&log_ctx->rb, bfr_sz) != KS_OK)
    {
        free(log_ctx);
        return KS_ERR;
    }

    (*ctx) = log_ctx;

    return KS_OK;
}

int ks_log_create(struct ks_log_ctx *ctx,
                 struct ks_log_object **obj,
                 const char *name)
{
    if (ctx == NULL)
        return KS_ERR;

    if (ctx->objects == NULL)
    {
        ctx->objects = malloc(sizeof(struct ks_log_object));
        memset(ctx->objects, 0, sizeof(struct ks_log_object));
        ctx->objects->name = strdup(name);
        ctx->objects->log_ctx = ctx;
        (*obj) = ctx->objects;
    }
    else
    {
        struct ks_log_object *last = ctx->objects;

        while (last->next)
            last = last->next;

        last->next = malloc(sizeof(struct ks_log_object));
        memset(last->next, 0, sizeof(struct ks_log_object));
        last->next->name = strdup(name);
        last->next->log_ctx = ctx;
        (*obj) = last->next;
    }
    
    return KS_OK;
}

int ks_log_add_source(struct ks_log_object *obj,
                      enum ks_log_level lvl,
                      size_t buf_sz,
                      int fd)
{
    struct ks_eventloop_ctx *ctx = obj->log_ctx->el;
    struct ks_eventloop_io *io = ks_eventloop_alloc();


    struct ks_log_source *src = malloc(sizeof(struct ks_log_source));

    memset(src, 0, sizeof(struct ks_log_source));

    src->log_object = obj;
    src->next = NULL;
    src->lvl = lvl;
    src->input_buf = malloc(buf_sz);
    src->input_buf_sz = buf_sz;

    io->fd = fd;
    io->data = src;
    io->cb = __ks_log_in_cb;
    io->flags = EPOLLIN;

    if (obj->sources == NULL)
    {
        obj->sources = src;
    }
    else
    {
        struct ks_log_source *last = obj->sources;

        while (last->next)
            last = last->next;

        last->next = src;
    }

    return ks_eventloop_add(ctx, io);
}

int ks_log_add_sink(struct ks_log_ctx *log, int fd)
{
    struct ks_eventloop_ctx *ctx = log->el;
    struct ks_eventloop_io *io = ks_eventloop_alloc();
    struct ks_log_sink *sink = malloc(sizeof(struct ks_log_sink));

    memset(sink, 0, sizeof(struct ks_log_sink));
    sink->io = io;
    sink->log_ctx = log;
    sink->next = NULL;
    int rc = ks_ringbuffer_new_tail(log->rb, &sink->t);

    if (rc != KS_OK)
    {
        free(sink->t);
        free(sink);
        return rc;
    }

    io->fd = fd;
    io->data = sink;
    io->cb = __ks_log_out_cb;
    io->flags = 0;

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
    }

    return ks_eventloop_add(ctx, io);
}
