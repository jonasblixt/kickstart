#ifndef INCLUDE_KICKSTART_EVENTLOOP_H_
#define INCLUDE_KICKSTART_EVENTLOOP_H_

#include <kickstart/kickstart.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

#ifndef KS_EVENTLOOP_MAX_EVENTS
#define KS_EVENTLOOP_MAX_EVENTS 256
#endif

struct ks_eventloop_io;

typedef void (*ks_eventloop_cb_t) (void *data, struct ks_eventloop_io *io);

struct ks_eventloop_ctx
{
    int ep_fd;
    bool run;
};

struct ks_eventloop_io
{
    ks_eventloop_cb_t cb;
    void *data;
    int fd;
    int flags;
};

int ks_eventloop_init(struct ks_eventloop_ctx *ctx);
int ks_eventloop_stop(struct ks_eventloop_ctx *ctx);
struct ks_eventloop_io * ks_eventloop_alloc(void);
int ks_eventloop_add(struct ks_eventloop_ctx *ctx,
                     struct ks_eventloop_io *io);
int ks_eventloop_remove(struct ks_eventloop_ctx *ctx,
                        struct ks_eventloop_io *io);

int ks_eventloop_loop_once(struct ks_eventloop_ctx *ctx, int timeout_ms);
int ks_eventloop_loop(struct ks_eventloop_ctx *ctx);

#endif  // INCLUDE_KICKSTART_EVENTLOOP_H_
