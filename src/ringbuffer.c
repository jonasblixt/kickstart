#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <kickstart/ringbuffer.h>

int ks_ringbuffer_init(struct ks_ringbuffer **new_rb, size_t sz)
{
    struct ks_ringbuffer *rb;
    int rc;

    if (!new_rb)
        return KS_ERR;
    if (!sz)
        return KS_ERR;

    rb = malloc(sizeof(struct ks_ringbuffer));

    if (!rb)
    {
        rc = KS_ERR;
        goto ringbuffer_init_err1;
    }

    memset(rb, 0, sizeof(struct ks_ringbuffer));

    rb->bfr = malloc(sz);
    rb->bfr_sz = sz;

    if (rb->bfr == NULL)
    {
        rc = KS_ERR;
        goto ringbuffer_init_err2;
    }

    if (ks_ll_init(&rb->tails) != KS_OK)
    {
        rc = KS_ERR;
        goto err_free_buffer;
    }

    *new_rb = rb;
    return KS_OK;

err_free_buffer:
    free(rb->bfr);
ringbuffer_init_err2:
    free(rb);
ringbuffer_init_err1:
    return rc;
}

int ks_ringbuffer_free(struct ks_ringbuffer *rb)
{
    if (!rb)
        return KS_ERR;

    if (ks_ll_free(&rb->tails))
        return KS_ERR;

    if (rb->bfr)
        free(rb->bfr);

    free(rb);

    return KS_OK;
}

int ks_ringbuffer_new_tail(struct ks_ringbuffer *rb,
                           struct ks_ringbuffer_tail **new_t)
{
    struct ks_ringbuffer_tail *t;

    if (!rb)
        return KS_ERR;
    if (!new_t)
        return KS_ERR;

    if (ks_ll_append2(rb->tails, (void **) &t,
                    sizeof(struct ks_ringbuffer_tail)) != KS_OK)
        return KS_ERR;

    t->rb = rb;
    *new_t = t;

    return KS_OK;
}

int ks_ringbuffer_remove_tail(struct ks_ringbuffer_tail *t)
{
    if (!t)
        return KS_ERR;

    struct ks_ringbuffer *rb = t->rb;
    struct ks_ll_item *item;
    ks_ll_data2item(rb->tails, t, &item);

    return ks_ll_remove(rb->tails, item);
}

int ks_ringbuffer_set_tail_advance_cb(struct ks_ringbuffer_tail *t,
                                      ks_ringbuffer_tail_advance_t cb)
{
    if (!t)
        return KS_ERR;
    if (!cb)
        return KS_ERR;

    t->tail_advance_cb = cb;

    return KS_OK;
}

static int process_tails(struct ks_ringbuffer *rb, uint64_t start,
                         uint64_t stop)
{

    ks_ll_foreach(rb->tails, i)
    {
        struct ks_ringbuffer_tail *t = (struct ks_ringbuffer_tail *) i->data;

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
    if (!rb)
        return KS_ERR;
    if (!data)
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

        ks_ll_foreach(rb->tails, i)
        {
            struct ks_ringbuffer_tail *t =
                        (struct ks_ringbuffer_tail *) i->data;
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

        ks_ll_foreach(rb->tails, i)
        {
            struct ks_ringbuffer_tail *t =
                        (struct ks_ringbuffer_tail *) i->data;

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
    uint64_t tail_index;
    uint64_t available = 0;

    if (!rb)
        return KS_ERR;
    if (!t)
        return KS_ERR;
    if (sz > rb->bfr_sz)
        return KS_ERR;
    if (sz > t->available)
        return KS_ERR;

    tail_index = t->tail_index;

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
