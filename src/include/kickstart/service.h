#ifndef INCLUDE_KICKSTART_SERVICE_H_
#define INCLUDE_KICKSTART_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

enum service_state
{
    KS_SVC_INIT,
    KS_SVC_WATINING,
    KS_SVC_AVAILABLE,
    KS_SVC_STOPPED,
    KS_SVC_CRASH_HOLDOFF
};

struct
{
    pid_t pid;
    int restarts;
    int exit_result;
    enum service_state state;
} ks_service_status;

struct ks_service;

struct
{
    const char *description;
    const char *path;
    // args
    struct ks_service *next;
} ks_service;

struct ks_service * ks_service_init(void);
int ks_service_start(struct ks_service *svc);
int ks_service_stop(struct ks_service *svc);

#endif
