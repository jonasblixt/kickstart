#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <kickstart/ringbuffer.h>

int ks_ringbuffer_init(struct ks_ringbuffer **rb, size_t sz)
{
    *rb = malloc(sizeof(struct ks_ringbuffer));

    if ((*rb) == NULL)
        goto ringbuffer_init_err1;
    memset((*rb), 0, sizeof(struct ks_ringbuffer));

    (*rb)->bfr = malloc(sz);
    (*rb)->bfr_sz = sz;

    if ((*rb)->bfr == NULL)
    {
        goto ringbuffer_init_err2;
    }

    return KS_OK;

ringbuffer_init_err2:
    free(*rb);
ringbuffer_init_err1:
    return KS_ERR;
}

int ks_ringbuffer_free(struct ks_ringbuffer *rb)
{
    if (rb == NULL)
        return KS_ERR;

    free(rb);

    return KS_OK;
}

int ks_ringbuffer_new_tail(struct ks_ringbuffer *rb,
                           struct ks_ringbuffer_tail **t)
{
    if (rb == NULL)
        return KS_ERR;

    if (rb->tails == NULL)
    {
        rb->tails = malloc(sizeof(struct ks_ringbuffer_tail));
        if (rb->tails == NULL)
            return KS_ERR;
        memset(rb->tails, 0, sizeof(struct ks_ringbuffer_tail));
        (*t) = rb->tails;
        return KS_OK;
    }

    struct ks_ringbuffer_tail *last = rb->tails;

    for (;last->next;last = last->next) {};

    last->next = malloc(sizeof(struct ks_ringbuffer_tail));

    if (last->next == NULL)
        return KS_ERR;

    memset(last->next, 0, sizeof(struct ks_ringbuffer_tail));
    (*t) = last->next;

    return KS_OK;
}

int ks_ringbuffer_set_tail_advance_cb(struct ks_ringbuffer_tail *t,
                                      ks_ringbuffer_tail_advance_t cb)
{
    if (t == NULL)
        return KS_ERR;
    if (cb == NULL)
        return KS_ERR;

    t->tail_advance_cb = cb;

    return KS_OK;
}

static int process_tails(struct ks_ringbuffer *rb, uint64_t start,
                         uint64_t stop)
{

    for (struct ks_ringbuffer_tail *t = rb->tails; t != NULL; t = t->next)
    {
        if (t->tail_index == rb->head_index)
            continue;

        if ((t->tail_index >= start) && (t->tail_index <= stop))
        {
            uint64_t new_tail_index = stop;

            if (t->tail_advance_cb != NULL)
                new_tail_index = t->tail_advance_cb(rb, t, stop);

            t->truncated_bytes += new_tail_index - t->tail_index;
            t->tail_index = new_tail_index;
        }
    }
    return KS_OK;
}

int ks_ringbuffer_write(struct ks_ringbuffer *rb, const char *data, size_t sz)
{
    if (rb == NULL)
        return KS_ERR;
    if (data == NULL)
        return KS_ERR;

    uint64_t available1 = rb->bfr_sz - rb->head_index;
    char *head = &rb->bfr[rb->head_index];

    if (sz > rb->bfr_sz)
        return KS_ERR;

    if (sz <= available1)
    {
        memcpy(head, data, sz);
        process_tails(rb, rb->head_index, rb->head_index + sz);
        rb->head_index = (rb->head_index+sz)%rb->bfr_sz;

        for (struct ks_ringbuffer_tail *t = rb->tails; t != NULL; t = t->next)
        {
            t->available = t->available + sz;

            if (t->available > rb->bfr_sz)
                t->available = rb->bfr_sz;
        }
    }
    else
    {
        memcpy(head, data, available1);
        process_tails(rb, rb->head_index, rb->bfr_sz-1);
        uint64_t remainder = sz - available1;
        process_tails(rb, 0, remainder);
        memcpy(rb->bfr, &data[available1], remainder);
        rb->head_index = remainder;

        for (struct ks_ringbuffer_tail *t = rb->tails; t != NULL; t = t->next)
        {
            t->available = t->available + available1 + remainder;

            if (t->available > rb->bfr_sz)
                t->available = rb->bfr_sz;
        }
    }

    return KS_OK;
}

int ks_ringbuffer_read(struct ks_ringbuffer *rb, struct ks_ringbuffer_tail *t,
                       char *data, size_t sz)
{
    uint64_t tail_index = t->tail_index;
    uint64_t available = 0;
    
    if (rb == NULL)
        return KS_ERR;
    if (t == NULL)
        return KS_ERR;
    if (sz > rb->bfr_sz)
        return KS_ERR;
    if (sz > t->available)
        return KS_ERR;

    if (tail_index < rb->head_index)
    {
        available = rb->head_index - tail_index;

        if (available < sz)
            return KS_ERR;

        memcpy(data, &rb->bfr[tail_index], sz);
        t->tail_index = (t->tail_index+sz)%rb->bfr_sz;
        t->available -= sz;
    }
    else
    {
        uint64_t chunk1_sz = rb->bfr_sz - tail_index;
        uint64_t chunk2_sz = rb->head_index;

        available = chunk1_sz + chunk2_sz;

        if (sz > available)
            return KS_ERR;
    
        if (sz <= chunk1_sz)
        {
            memcpy(data, &rb->bfr[tail_index], sz);
            t->tail_index = (t->tail_index+sz)%rb->bfr_sz;
            t->available -= sz;
        }
        else
        {
            memcpy(data, &rb->bfr[tail_index], chunk1_sz);
            
            uint64_t remainder = sz - chunk1_sz;
            memcpy(&data[chunk1_sz], rb->bfr,
                   remainder);

            t->tail_index = remainder;
            t->available -= (chunk1_sz + remainder);
        }
    }

    return KS_OK;
}
