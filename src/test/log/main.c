#include <stdio.h>
#include <nala.h>
#include <unistd.h>
#include <kickstart/log.h>
#include <kickstart/eventloop.h>
#include <kickstart/ringbuffer.h>

#ifndef __NALA
#include <test/log/nala_mocks.h>
#endif
/*
 * Planned tests for core logging functionallity:
 *
 *  o Multiple sinks
 *  o Event roll-over
 *  o Test too large messages
 *  o Corrupt header
 *  o Buffer level callbacks per sink
 *  o Flushing
 *  o Error collection
 *  o Error callbacks
 *
 * */

static int test_input_formatter(struct ks_log_source *src,
                                uint16_t sz,
                                struct ks_log_entry_header *hdr)
{
    hdr->magic = KS_LOG_HEADER_MAGIC;
    hdr->sz = sz;
    hdr->source_id = src->source_id;
    hdr->ts.tv_sec = 1;
    hdr->ts.tv_nsec = 0;
    hdr->lvl = KS_LOG_LEVEL_INFO;

    return KS_OK;
}


static int test_output_formatter(struct ks_log_sink *sink,
                                 uint16_t sz,
                                 uint16_t *new_sz,
                                 struct ks_log_entry_header *hdr)
{
    int len;
    memcpy(sink->tmp, sink->buf, sz);

    len = snprintf(sink->buf, KS_LOG_OUTPUT_BUF_SZ,
             "%li %s %s: ",
             hdr->ts.tv_sec,
             ks_log_level_to_string(hdr->lvl),
             ks_log_source_id_to_string(sink->log, hdr->source_id));
    
    if (len && (len + sz < KS_LOG_OUTPUT_BUF_SZ))
    {
        memcpy(sink->buf + len, sink->tmp, sz);
        sink->buf[len+sz] = 0;
    }
    else
    {
        return KS_ERR;
    }

    (*new_sz) = strlen(sink->buf);

    return KS_OK;
}

TEST(log_basic)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx *ctx;
    struct ks_log *log;
    struct ks_log_sink *sink0;
    struct ks_log_source *src0;
    const char *msg = "Hello\n";

    pipe(fds);

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 1024);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_source(log, &src0, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_input_formatter(src0, test_input_formatter);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_source_name(src0, "test");
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, &sink0, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_output_formatter(sink0, test_output_formatter);
    write(fds[1], msg, 7);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    CAPTURE_OUTPUT(message, msgs_stderr)
    {
        rc = ks_eventloop_loop_once(ctx, 500);
        ASSERT_EQ(rc, KS_OK);
    }

    ASSERT_EQ(message, "1 INFO test: Hello\n");

    rc = ks_log_free_source(src0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free_sink(sink0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}


TEST(log_double_free)
{
    struct ks_eventloop_ctx *ctx;
    struct ks_log *log;
    int rc;

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 1024);
    ASSERT_EQ(rc, KS_OK);


    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}

TEST(log_nullargs)
{
    int p;
    int rc;

    rc = ks_log_init(NULL, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_init((struct ks_log **)&p, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_init((struct ks_log **)&p, (struct ks_eventloop_ctx *)&p, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_set_input_formatter(NULL, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_set_input_formatter((struct ks_log_source *) &p, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_set_output_formatter(NULL, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_set_output_formatter((struct ks_log_sink *) &p, NULL);
    ASSERT_EQ(rc, KS_ERR);

    char *c;

    c = (char *) ks_log_level_to_string(0);
    ASSERT_EQ(c, "INVALID");

    c = (char *) ks_log_level_to_string(99999);
    ASSERT_EQ(c, "INVALID");

    rc = ks_log_add_source(NULL, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);

    struct ks_log l;
    rc = ks_log_add_source(&l, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);
}


TEST(log_overlap)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx *ctx;
    struct ks_log *log;
    struct ks_log_sink *sink0;
    struct ks_log_source *src0;
    const char *msg = "Hello\n";

    pipe(fds);

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 64);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_source(log, &src0, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_input_formatter(src0, test_input_formatter);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_source_name(src0, "test");
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, &sink0, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_output_formatter(sink0, test_output_formatter);
    write(fds[1], msg, 7);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    write(fds[1], msg, 7);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    CAPTURE_OUTPUT(message, msgs_stderr)
    {
        rc = ks_eventloop_loop_once(ctx, 500);
        ASSERT_EQ(rc, KS_OK);
    }

    ASSERT_EQ(message, "1 INFO test: Hello\n");


    rc = ks_log_free_source(src0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free_sink(sink0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}


TEST(log_structured_input)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx *ctx;
    struct ks_log *log;
    struct ks_log_sink *sink0;
    struct ks_log_source *src0;
    struct ks_log_entry_header hdr;
    const char *msg = "Hello\n";

    pipe(fds);

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 64);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_source(log, &src0, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_source_name(src0, "test");
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, &sink0, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_output_formatter(sink0, test_output_formatter);
    hdr.magic = KS_LOG_HEADER_MAGIC;
    hdr.sz = 7;
    hdr.source_id = src0->source_id;
    hdr.ts.tv_sec = 1;
    hdr.ts.tv_nsec = 0;
    hdr.lvl = KS_LOG_LEVEL_INFO;

    write(fds[1], &hdr, sizeof(hdr));
    write(fds[1], msg, 7);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    CAPTURE_OUTPUT(message, msgs_stderr)
    {
        rc = ks_eventloop_loop_once(ctx, 500);
        ASSERT_EQ(rc, KS_OK);
    }

    ASSERT_EQ(message, "1 INFO test: Hello\n");

    rc = ks_log_free_source(src0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free_sink(sink0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}


TEST(log_incorrect_hdr_magic)
{
    int rc;
    int fds[2];
    struct ks_eventloop_ctx *ctx;
    struct ks_log *log;
    struct ks_log_sink *sink0;
    struct ks_log_source *src0;
    struct ks_log_entry_header hdr;
    const char *msg = "Hello\n";

    pipe(fds);

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 64);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_source(log, &src0, fds[0]);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_source_name(src0, "test");
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_add_sink(log, &sink0, STDOUT_FILENO);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_set_output_formatter(sink0, test_output_formatter);
    hdr.magic = KS_LOG_HEADER_MAGIC+1;
    hdr.sz = 7;
    hdr.source_id = src0->source_id;
    hdr.ts.tv_sec = 1;
    hdr.ts.tv_nsec = 0;
    hdr.lvl = KS_LOG_LEVEL_INFO;

    write(fds[1], &hdr, sizeof(hdr));
    write(fds[1], msg, 7);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_loop_once(ctx, 500);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free_source(src0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free_sink(sink0);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_free(&log);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}

TEST(log_init_fail1)
{
    int rc;
    struct ks_log *log;
    struct ks_eventloop_ctx *ctx;

    rc = ks_eventloop_init(&ctx);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_log_init(&log, ctx, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_init(&log, NULL, 16);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_init(NULL, ctx, 16);
    ASSERT_EQ(rc, KS_ERR);

    ks_ringbuffer_init_mock_once(16, KS_ERR);
    rc = ks_log_init(&log, ctx, 16);
    ASSERT_EQ(rc, KS_ERR);
    ks_ringbuffer_init_mock_disable();


    ks_ll_init_mock_none();
    ks_ll_init_mock_none();
    ks_ll_init_mock_once(KS_ERR);
    rc = ks_log_init(&log, ctx, 16);
    ASSERT_EQ(rc, KS_ERR);
    ks_ll_init_mock_disable();

    rc = ks_eventloop_free(&ctx);
    ASSERT_EQ(rc, KS_OK);
}

TEST(log_add_sink_fail)
{
    int rc;
    struct ks_log l1;
    struct ks_log_sink *s1;

    rc = ks_log_add_sink(NULL, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_add_sink(&l1, NULL, 0);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_add_sink(&l1, &s1, 0);
    ASSERT_EQ(rc, KS_ERR);

    ks_eventloop_alloc_io_mock_once(KS_ERR);
    rc = ks_log_add_sink(&l1, &s1, 1);
    ASSERT_EQ(rc, KS_ERR);

}

TEST(log_free_fail)
{
    int rc;
    struct ks_log *l = NULL;

    rc = ks_log_free(NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_log_free(&l);
    ASSERT_EQ(rc, KS_ERR);

    l = (struct ks_log *) 1;

    ks_ll_free_mock_once(KS_ERR);
    rc = ks_log_free(&l);
    ASSERT_EQ(rc, KS_ERR);
    ks_ll_free_mock_disable();

    l = malloc(sizeof(struct ks_log));
    ks_ringbuffer_free_mock_once(KS_ERR);
    ks_ll_free_mock(KS_OK);
    rc = ks_log_free(&l);
    ASSERT_EQ(rc, KS_ERR);
    ks_ringbuffer_free_mock_disable();
    free(l);


    l = malloc(sizeof(struct ks_log));
    ks_ll_free_mock_once(KS_OK);
    ks_ll_free_mock_once(KS_ERR);
    rc = ks_log_free(&l);
    ASSERT_EQ(rc, KS_ERR);
    ks_ll_free_mock_disable();

    free(l);
}

