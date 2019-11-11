#ifndef INCLUDE_KICKSTART_RINGBUFFER_H_
#define INCLUDE_KICKSTART_RINGBUFFER_H_

#include <stdint.h>
#include <kickstart/kickstart.h>

struct ks_ringbuffer_tail;
struct ks_ringbuffer;

typedef uint64_t (*ks_ringbuffer_tail_advance_t) (struct ks_ringbuffer *rb,
                                                  struct ks_ringbuffer_tail *t,
                                                  uint64_t new_index);
struct ks_ringbuffer_tail
{
    uint64_t tail_index;
    uint64_t truncated_bytes;
    uint64_t available;
    ks_ringbuffer_tail_advance_t tail_advance_cb;
    struct ks_ringbuffer_tail *next;
};

struct ks_ringbuffer
{
    char *bfr;
    size_t bfr_sz;
    uint64_t head_index;
    struct ks_ringbuffer_tail *tails;
};

int ks_ringbuffer_init(struct ks_ringbuffer **rb, size_t sz);
int ks_ringbuffer_free(struct ks_ringbuffer *rb);
int ks_ringbuffer_new_tail(struct ks_ringbuffer *rb,
                           struct ks_ringbuffer_tail **t);
int ks_ringbuffer_set_tail_advance_cb(struct ks_ringbuffer_tail *t,
                                      ks_ringbuffer_tail_advance_t cb);

int ks_ringbuffer_write(struct ks_ringbuffer *rb, const char *data, size_t sz);
int ks_ringbuffer_read(struct ks_ringbuffer *rb, struct ks_ringbuffer_tail *t,
                       char *data, size_t sz);

#endif  // INCLUDE_KICKSTART_RINGBUFFER_H_
