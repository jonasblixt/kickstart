#include <stdlib.h>
#include <kickstart/eventloop.h>


int ks_eventloop_init(struct ks_eventloop_ctx *ctx)
{
    ctx->run = false;
    ctx->ep_fd = epoll_create(KS_EVENTLOOP_MAX_EVENTS);

    if (ctx->ep_fd == -1)
        return KS_ERR;

    ctx->run = true;
    return KS_OK;
}

int ks_eventloop_stop(struct ks_eventloop_ctx *ctx)
{
    if (ctx->run && (ctx != NULL))
    {
        ctx->run = false;
        return KS_OK;
    }

    return KS_ERR;
}

int ks_eventloop_remove(struct ks_eventloop_ctx *ctx,
                        struct ks_eventloop_io *io)
{
    int rc = KS_OK;

    if (ctx == NULL || io == NULL)
    {
        rc = KS_ERR;
        goto eventloop_remove_err_out;
    }
    
    if (epoll_ctl(ctx->ep_fd, EPOLL_CTL_DEL, io->fd, NULL) == -1)
        rc = KS_ERR;

    free(io);
eventloop_remove_err_out:
    return rc;
}

struct ks_eventloop_io * ks_eventloop_alloc(void)
{
    return malloc(sizeof(struct ks_eventloop_io));
}

int ks_eventloop_add(struct ks_eventloop_ctx *ctx,
                     struct ks_eventloop_io *io)
{
    struct epoll_event ev;
    int err = KS_OK;

    if (io == NULL || ctx == NULL)
    {
        err = KS_ERR;
        goto eventloop_add_err_out;
    }

    if ((io->fd < 0) || (io->cb == NULL))
    {
        err = KS_ERR;
        goto eventloop_add_err_out;
    }

    ev.events = io->flags;
    ev.data.ptr = io;

    if (epoll_ctl(ctx->ep_fd, EPOLL_CTL_ADD, io->fd, &ev) == -1)
    {
        err = KS_ERR;
        goto eventloop_add_err_out;
    }

eventloop_add_err_out:
    return err;
}

int ks_eventloop_loop_once(struct ks_eventloop_ctx *ctx, int timeout_ms)
{
    struct epoll_event ev;
    struct ks_eventloop_io *io;

    if (ctx == NULL)
        return KS_ERR;

    if (timeout_ms <= 0)
        return KS_ERR;

    int nr_of_events = epoll_wait(ctx->ep_fd, &ev, 1, timeout_ms);

    if (nr_of_events != 1)
        return KS_ERR;

    io = (struct ks_eventloop_io *) ev.data.ptr;

    if (io->cb == NULL)
        return KS_ERR;

    io->cb(io->data, io);

    return KS_OK;
}

int ks_eventloop_loop(struct ks_eventloop_ctx *ctx)
{
    int no_of_events = 0;
    struct epoll_event ev[KS_EVENTLOOP_MAX_EVENTS];
    struct ks_eventloop_io *io = NULL;

    while (ctx->run)
    {
        no_of_events = epoll_wait(ctx->ep_fd, ev,
                                    KS_EVENTLOOP_MAX_EVENTS, 500);

        for (int i = 0; i < no_of_events; i++)
        {
            io = (struct ks_eventloop_io *) ev[i].data.ptr;

            if (io->cb != NULL)
                io->cb(io->data, io);
        }
    }

    return KS_OK;
}
