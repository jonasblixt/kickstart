#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <kickstart/log.h>

const char *ks_log_level_str[] =
{
    "INFO   ",
    "WARNING",
    "ERROR  ",
};

static void __ks_log_in_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_object *obj = io->data;
    struct ks_log_ctx *ctx = obj->log_ctx;
    char tmp[64];
    int start_index = ctx->head_index;
    int available = ctx->bfr_sz - start_index;
    char *head = &ctx->bfr[start_index];

    ctx->dbg_in++;

    snprintf(tmp, 64, "%6.6f %s %s", 0.0, obj->name,
                                          ks_log_level_str[0]);
    int prefix_sz = strlen(tmp);
    int data_to_write = prefix_sz > available?available:prefix_sz;

    memcpy(head, tmp, prefix_sz);
    ctx->head_index += prefix_sz;

    /* Check for tails within the range start_index and start_idx+r_bytes*/
    for (struct ks_log_sink *sink = ctx->sinks; sink != NULL; sink = sink->next)
    {
        if (sink->tail_index > start_index)
        {
            if ((sink->tail_index + prefix_sz) > ctx->bfr_sz)
                sink->tail_index = (sink->tail_index + prefix_sz) % ctx->bfr_sz;
            else
                sink->tail_index += prefix_sz;
        }
        sink->io->flags = EPOLLOUT;
        ks_eventloop_io_oneshot(ctx->el, sink->io);
    }

    head = &ctx->bfr[ctx->head_index];


    size_t r_bytes = read(io->fd, head, available);
 
    /* Advance head index */
    ctx->head_index += r_bytes;

    /* Check for tails within the range start_index and start_idx+r_bytes*/
    for (struct ks_log_sink *sink = ctx->sinks; sink != NULL; sink = sink->next)
    {
        if (sink->tail_index > start_index)
        {
            if ((sink->tail_index + r_bytes) > ctx->bfr_sz)
                sink->tail_index = (sink->tail_index + r_bytes) % ctx->bfr_sz;
            else
                sink->tail_index += r_bytes;
        }
        sink->io->flags = EPOLLOUT;
        ks_eventloop_io_oneshot(ctx->el, sink->io);
    }

    if (r_bytes == available)
        ctx->head_index = 0;
}

static void __ks_log_out_cb(void *data, struct ks_eventloop_io *io)
{
    struct ks_log_sink *sink = io->data;
    struct ks_log_ctx *ctx = sink->log_ctx;
    char *tail = &ctx->bfr[sink->tail_index];
    int available = 0;
    size_t written = 0;

    ctx->dbg_out++;

    if (sink->tail_index <= ctx->head_index)
    {
        available = ctx->head_index - sink->tail_index;
        written = write(STDOUT_FILENO, tail, available);
        sink->tail_index += written;

        if (written != available)
            ks_eventloop_io_oneshot(ctx->el, io);

    } else {
        available = (ctx->bfr_sz - sink->tail_index);
        written = write(sink->io->fd, tail, available);
        sink->tail_index += written;

        if (written != available)
        {
            ks_eventloop_io_oneshot(ctx->el, io);
        } else {
            tail = &ctx->bfr[sink->tail_index];
            available = ctx->head_index;
            written = write(sink->io->fd, tail, available);
            sink->tail_index = written;

            if (written != available)
                ks_eventloop_io_oneshot(ctx->el, io);
        }

    }
    

}

struct ks_log_ctx * ks_log_init(struct ks_eventloop_ctx *ctx,
                                size_t bfr_sz)
{
    struct ks_log_ctx *log_ctx = NULL;

    if (ctx == NULL)
        return NULL;

    log_ctx = malloc(sizeof(struct ks_log_ctx));

    if (log_ctx == NULL)
        return NULL;

    memset(log_ctx, 0, sizeof(struct ks_log_ctx));
    log_ctx->el = ctx;
    log_ctx->bfr = malloc(bfr_sz);
    log_ctx->bfr_sz = bfr_sz;
    log_ctx->head_index = 0;

    if (log_ctx->bfr == NULL)
        free(log_ctx);

    return log_ctx;
}

struct ks_log_object * ks_log_create(struct ks_log_ctx *ctx,
                                     const char *name)
{
    if (ctx == NULL)
        return NULL;

    if (ctx->objects == NULL)
    {
        ctx->objects = malloc(sizeof(struct ks_log_object));
        memset(ctx->objects, 0, sizeof(struct ks_log_object));
        ctx->objects->name = strdup(name);
        ctx->objects->log_ctx = ctx;
        return ctx->objects;
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
        return last->next;
    }
}

int ks_log_add_source(struct ks_log_object *obj,
                      enum ks_log_level lvl,
                      int fd)
{
    struct ks_eventloop_ctx *ctx = obj->log_ctx->el;
    struct ks_eventloop_io *io = ks_eventloop_alloc();

    io->fd = fd;
    io->data = obj;
    io->cb = __ks_log_in_cb;
    io->flags = EPOLLIN;

    struct ks_log_source *src = malloc(sizeof(struct ks_log_source));

    src->io = io;
    src->next = NULL;
    src->lvl = lvl;

    if (obj->sources == NULL)
    {
        obj->sources = src;
    } else {
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

    sink->io = io;
    sink->log_ctx = log;
    sink->next = NULL;
    sink->tail_index = 0;

    io->fd = fd;
    io->data = sink;
    io->cb = __ks_log_out_cb;
    io->flags = 0;

    if (log->sinks == NULL)
    {
        log->sinks = sink;
    } else {
        struct ks_log_sink *last = log->sinks;

        while (last->next)
            last = last->next;
        last->next = sink;
    }

    return ks_eventloop_add(ctx, io);
}
