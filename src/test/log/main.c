#include <narwhal.h>
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
    log = ks_log_init(&ctx, 1024);
    ASSERT(log != NULL);

    struct ks_log_object *obj = ks_log_create(log, "test");
    ASSERT (obj != NULL);

    rc = ks_log_add_source(obj, KS_LOG_LEVEL_INFO, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(log->head_index, 0);
    ASSERT_EQ(log->sinks->tail_index, 0);

    write(fds[1], msg, 6);

    rc = ks_eventloop_loop_once(&ctx,500);
    ASSERT_EQ(rc, KS_OK);

    CAPTURE_OUTPUT(message, msgs_stderr)
    {
        rc = ks_eventloop_loop_once(&ctx,500);
        ASSERT_EQ(rc, KS_OK);
    }

    ASSERT_EQ(log->dbg_out, 1);
    ASSERT_EQ(log->dbg_in, 1);
    ASSERT_EQ(log->head_index, 27);
    ASSERT_EQ(log->sinks->tail_index, 27);
    ASSERT_EQ(message, "0.000000 test INFO   Hello");

    rc = ks_eventloop_loop_once(&ctx,5);
    ASSERT_EQ(rc, KS_ERR);
    ASSERT_EQ(log->dbg_out, 1);
    ASSERT_EQ(log->dbg_in, 1);
}
