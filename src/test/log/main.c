#include <nala.h>
#include <unistd.h>
#include <kickstart/log.h>
#include <kickstart/eventloop.h>
#include <kickstart/ringbuffer.h>

TEST(log_basic)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx ctx;
    const char *msg = "Hello";

    pipe(fds);

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    struct ks_log_ctx *log;
    rc = ks_log_init(&ctx, &log, 1024);
    ASSERT_EQ(rc, KS_OK);

    struct ks_log_object *obj;
    rc = ks_log_create(log, &obj, "test");
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_source(obj, KS_LOG_LEVEL_INFO, 256, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);

    write(fds[1], msg, 6);

    rc = ks_eventloop_loop_once(&ctx,500);
    ASSERT_EQ(rc, KS_OK);
/*
    CAPTURE_OUTPUT(message, msgs_stderr)
    {
        rc = ks_eventloop_loop_once(&ctx,500);
        ASSERT_EQ(rc, KS_OK);
    }

    ASSERT_EQ(message, "0.000000 test INFO   Hello");

    rc = ks_eventloop_loop_once(&ctx,5);
    ASSERT_EQ(rc, KS_ERR);*/
}
