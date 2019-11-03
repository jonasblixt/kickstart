#include <narwhal.h>
#include <kickstart/log.h>

/**
 * Logging module:
 *
 * Main logging ideas:
 *  o Collect log data from processes
 *      - Connect stdout to info logs on log target
 *      - Connect stderr to err logs on log target
 *  o Collect kernel logs from /dev/kmsg
 *  o Collect logs from TEE / TA's
 *  o Internal logging target(s)
 *  o Rate limiting
 *  o Internal ram buffer multiple of disk erase block / allocation size
 *  o Commit to disk when ram buffer is full or on flush command
 *  o Read log entries from all targets or specific targets
 *      - All entries, ram-buffer including persistent logs
 *      - Read last <n> entries
 *      - Read last <n> seconds/minutes
 *  o Support for aggregating logs from multiple VM's co-processors
 *
 *  o Log server (consumes log entries)
 *      - Owns ram log
 *      - Owns peristent storage
 *      - Support listeners
 *  o Log client (Produces log entries)
 *
 * Dependencies:
 *  o Basic async functionality
 *
 * API:
 *
 *  struct ks_log * ks_log_init(char *bfr, sz)
 *  struct ks_log_object * ks_add_log_object(ks_log *, const char *name)
 *
 *  int ks_log_add_fd_source(ks_log_object *src, KS_INFO, int fd)
 *  int ks_log_add_file_source(ks_log_object *src, KS_INFO, const char *fn)
 *  int ks_log_free_source(ks_log_source *src)
 *
 *  ks_log_write(ks_log_object *src, KS_LOG_INFO, const char *, ...)
 *  ks_log_flush(ks_log *log);
 *  ks_log_exit(ks_log *log);
 *  
 * Data model:
 *
 *  struct ks_log
 *  {
 *      char *bfr;
 *      size_t bfr_size;
 *      struct ks_log_object *objects;
 *  }
 *
 *  struct ks_log_entry
 *  {
 *      struct ks_log_object *obj
 *      int ts_usec
 *      const char *msg
 *      size_t msg_sz
 *  }
 *
 *  struct ks_log_object
 *  {
 *      struct ks_log *l
 *      const char *name
 *      struc ks_log_source *sources;
 *      struct ks_log_object *next;
 *      struct ks_log_entry *entries;
 *  }
 *
 *  struct ks_log_source
 *  {
 *      struct ks_log_object *obj
 *      enum log level
 *      int fd
 *      struct ks_log_source *next;
 *  }
 *
 */



