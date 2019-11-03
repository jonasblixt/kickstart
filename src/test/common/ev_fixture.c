#include <narwhal.h>
#include <kickstart/eventloop.h>
#include "ev_fixture.h"

TEST_FIXTURE(fixture_ctx, struct ks_eventloop_ctx *)
{
    int rc;
    *fixture_ctx = malloc(sizeof(struct ks_eventloop_ctx));
    ASSERT(fixture_ctx != NULL);

    rc = ks_eventloop_init(*fixture_ctx);
    ASSERT_EQ(rc, KS_OK);

    CLEANUP_FIXTURE(fixture_ctx)
    {
        int rc = ks_eventloop_stop(*fixture_ctx);
        ASSERT_EQ(rc, KS_OK);
        free(*fixture_ctx);
    }
}
