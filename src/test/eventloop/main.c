#include <nala.h>
#include <kickstart/eventloop.h>
#include <time.h>
#include <sys/timerfd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


static bool timer_cb_fired = false;

void timer_cb(void *data, struct ks_eventloop_io *io)
{
    const char *msg = (const char *) data;
    printf ("timer_cb called: %s\n", msg);
    timer_cb_fired = true;
}

/**
 *
 * Test that generates one event and call back
 *  using a timer fd
 */

TEST(ev_loop_process_one)
{
    struct ks_eventloop_ctx *ctx;
    int rc = -1;
	struct itimerspec ts;
	int msec = 10;
    int tfd;
    const char *test_data = "hello";

    timer_cb_fired = false;

    rc = ks_eventloop_init(&ctx);

    ASSERT_EQ(rc, KS_OK);

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    ASSERT(tfd > 0);

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	rc = timerfd_settime(tfd, 0, &ts, NULL);

    ASSERT_EQ(rc, 0);
    struct ks_eventloop_io *io;

    rc = ks_eventloop_alloc_io(ctx, &io);
    ASSERT_EQ(rc, KS_OK);

    io->fd = tfd;
    io->cb = timer_cb;
    io->data = (void *) test_data;
    io->flags = EPOLLIN;
    rc = ks_eventloop_add(ctx, io);

    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 10);

    ASSERT_EQ(rc, 0);
    ASSERT_EQ(timer_cb_fired, true);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}


TEST(ev_loop_remove_io)
{
    struct ks_eventloop_ctx *ctx;
    int rc = -1;
	struct itimerspec ts;
	int msec = 1;
    int tfd;
    const char *test_data = "hello";

    timer_cb_fired = false;

    rc = ks_eventloop_init(&ctx);

    ASSERT_EQ(rc, KS_OK);

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    
    ASSERT(tfd > 0);

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	rc = timerfd_settime(tfd, 0, &ts, NULL);
    ASSERT_EQ(rc, 0);

    struct ks_eventloop_io *io;
    rc = ks_eventloop_alloc_io(ctx, &io);
    ASSERT_EQ(rc, KS_OK);

    io->fd = tfd;
    io->cb = timer_cb;
    io->data = (void *) test_data;
    io->flags = EPOLLIN;
    rc = ks_eventloop_add(ctx, io);

    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 2);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(timer_cb_fired, true);

    rc = ks_eventloop_remove(ctx, io);
    ASSERT_EQ(rc, KS_OK);

	rc = timerfd_settime(tfd, 0, &ts, NULL);
    ASSERT_EQ(rc, KS_OK);

    timer_cb_fired = false;
    rc = ks_eventloop_loop_once(ctx, 2);
    ASSERT_EQ(rc, KS_ERR);
    ASSERT_EQ(timer_cb_fired, false);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}

TEST(ev_loop_process_one_timeout)
{
    struct ks_eventloop_ctx *ctx;
    int rc = -1;
	struct itimerspec ts;
	int msec = 10;
    int tfd;
    const char *test_data = "hello";

    timer_cb_fired = false;

    rc = ks_eventloop_init(&ctx);

    ASSERT_EQ(rc, KS_OK);

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    
    ASSERT(tfd > 0);

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	rc = timerfd_settime(tfd, 0, &ts, NULL);

    ASSERT_EQ(rc, 0);
    struct ks_eventloop_io *io;

    rc = ks_eventloop_alloc_io(ctx, &io);
    ASSERT_EQ(rc, KS_OK);

    io->fd = tfd;
    io->cb = timer_cb;
    io->data = (void *) test_data;
    io->flags = EPOLLIN;

    rc = ks_eventloop_add(ctx, io);

    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 5);

    ASSERT_EQ(rc, KS_ERR);
    ASSERT_EQ(timer_cb_fired, false);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}


TEST(ev_loop_stop)
{
    int rc;
    struct ks_eventloop_ctx *ctx;

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);
    rc = ks_eventloop_stop(ctx);
    ASSERT_EQ(rc, KS_OK);
    rc = ks_eventloop_stop(ctx);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}


void stop_timer_cb(void *data, struct ks_eventloop_io *io)
{
    int rc = ks_eventloop_stop((struct ks_eventloop_ctx*) data);
    ASSERT_EQ(rc, KS_OK);
}

TEST(ev_loop_stop2)
{
    struct ks_eventloop_ctx *ctx;
    int rc = -1;
	struct itimerspec ts;
	int msec = 2;
    int tfd;

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    
    ASSERT(tfd > 0);

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	rc = timerfd_settime(tfd, 0, &ts, NULL);

    ASSERT_EQ(rc, 0);

    struct ks_eventloop_io *io;
    rc = ks_eventloop_alloc_io(ctx, &io);
    ASSERT_EQ(rc, KS_OK);

    io->fd = tfd;
    io->cb = stop_timer_cb;
    io->data = ctx;
    io->flags = EPOLLIN;

    rc = ks_eventloop_add(ctx, io);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop(ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}

void test_pipe_cb(void *data, struct ks_eventloop_io *io)
{
    char bfr[16];
    bzero(bfr,sizeof(bfr));
    size_t read_bytes = read(io->fd, bfr, sizeof(bfr));
    ASSERT_MEMORY(bfr, "Hello world", read_bytes);
}

TEST(ev_loop_test_pipe)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx *ctx;
    const char msg[] = "Hello world";

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = pipe(fds);
    ASSERT_EQ(rc, 0);

    struct ks_eventloop_io *io;
    rc = ks_eventloop_alloc_io(ctx, &io);
    ASSERT_EQ(rc, KS_OK);

    io->fd = fds[0];
    io->cb = test_pipe_cb;
    io->flags = EPOLLIN;

    size_t written = write(fds[1], msg, sizeof(msg));
    ASSERT_EQ(written, sizeof(msg));

    rc = ks_eventloop_add(ctx, io);
    ASSERT_EQ(rc, KS_OK);


    rc = ks_eventloop_loop_once(ctx,500);
    ASSERT_EQ(rc, KS_OK);


    rc = ks_eventloop_remove(ctx, io);
    ASSERT_EQ(rc, KS_OK);

    close(fds[0]);
    close(fds[1]);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, 0);
}

