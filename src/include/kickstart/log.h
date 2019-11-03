#ifndef INCLUDE_KICKSTART_LOG_H_
#define INCLUDE_KICKSTART_LOG_H_

#include <kickstart/kickstart.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

struct ks_log;

struct
{
    struct ks_log *l;
    const char *name;
    char *bfr;
    size_t bfr_sz;
    struct ks_log_source *sources;
    struct ks_log_object *next;
    struct ks_log_entry *entries;
} ks_log_object;

struct
{
    struct ks_log_object *objects;
} ks_log;

int ks_log_init(struct ks_log *log);

#endif  // INCLUDE_KICKSTART_LOG_H_
