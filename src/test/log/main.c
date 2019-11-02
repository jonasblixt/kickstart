#include <narwhal.h>
#include <kickstart/log.h>

/**
 * General log requirements:
 * 
 *  o Support collecting logs from daemons
 *      - Redirected stdout/stderr file descriptors
 *  o Collect log from kernel
 *      - Open /dev/kmsg file descriptor
 *  o Internal log-messages
 *  o Fixed size ram buffer
 *  o Collect time reporting information
 *      - Kernel boot time
 *      - Initrd init time
 *      - Init time
 *  o Multiple file descriptors as input to the same log target
 *      - stderr produces ERR log entries
 *      - stdout produces INFO log entries
 *  o Sinks
 *      - RAM buffer
 *      - stdout
 *      - presistent storage
 * API:
 *
 *  ks_log_init(void)
 *  ks_create_log_target()
 *  ks_log_write(KS_LOG_INFO, const char *, ...)
 *
 *
 */



TEST(log_test_init)
{
    int rc = ks_log_init();

    ASSERT_EQ(rc, KS_OK);
}
