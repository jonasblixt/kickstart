#include <narwhal.h>
#include <kickstart/ringbuffer.h>

TEST(rb_test1)
{
    struct ks_ringbuffer *rb = NULL;
    int rc;
    
    rc = ks_ringbuffer_init(&rb, 1024);

    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_free(rb);

    ASSERT_EQ(rc, KS_OK);

}


TEST(rb_test_read_write_bytes1)
{
    struct ks_ringbuffer *rb = NULL;
    struct ks_ringbuffer_tail *t = NULL;
    char tmp[16];
    int rc;
    
    rc = ks_ringbuffer_init(&rb, 1024);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_GT(rb->head_index, 0);
    ASSERT_EQ(t->tail_index, 0);

    rc = ks_ringbuffer_read(rb, t, tmp, 5);
    ASSERT_EQ(rc, KS_OK);

    ASSERT_MEMORY(tmp, "Hello", 5);
    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 5);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);
}

TEST(rb_test_write_overlap)
{
    struct ks_ringbuffer *rb = NULL;
    struct ks_ringbuffer_tail *t = NULL;
    char tmp[16];
    int rc;
    
    rc = ks_ringbuffer_init(&rb, 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_GT(rb->head_index, 0);
    ASSERT_EQ(t->tail_index, 0);

    rc = ks_ringbuffer_read(rb, t, tmp, 5);
    ASSERT_EQ(rc, KS_OK);

    ASSERT_MEMORY(tmp, "Hello", 5);
    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 5);
    memset(tmp, 0, sizeof(tmp));

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 2);
    ASSERT_EQ(t->tail_index, 5);

    rc = ks_ringbuffer_read(rb, t, tmp, 5);
    ASSERT_EQ(rc, KS_OK);

    ASSERT_MEMORY(tmp, "Hello", 5);
    ASSERT_EQ(rb->head_index, 2);
    ASSERT_EQ(t->tail_index, 2);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);

}


TEST(rb_test_write_overlap2)
{
    struct ks_ringbuffer *rb = NULL;
    struct ks_ringbuffer_tail *t = NULL;
    char tmp[16];
    int rc;
    
    rc = ks_ringbuffer_init(&rb, 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 0);

    memset(tmp, 0, sizeof(tmp));

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 2);

    ASSERT_MEMORY(rb->bfr, "lolloHel", 8);
    ASSERT_EQ(t->tail_index, 2);
    ASSERT_EQ(t->truncated_bytes, 2);

    rc = ks_ringbuffer_read(rb, t, tmp, 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_MEMORY(tmp, "lloHello", 8);
    ASSERT_EQ(rb->head_index, 2);
    ASSERT_EQ(t->tail_index, 2);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);

}


TEST(rb_test_write_overlap3)
{
    struct ks_ringbuffer *rb = NULL;
    struct ks_ringbuffer_tail *t = NULL;
    struct ks_ringbuffer_tail *t2 = NULL;
    char tmp[16];
    int rc;
    
    rc = ks_ringbuffer_init(&rb, 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t2);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 0);

    memset(tmp, 0, sizeof(tmp));
    
    ASSERT_EQ(t->available, 5);
    ASSERT_EQ(t2->available, 5);
    rc = ks_ringbuffer_read(rb, t2, tmp, 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_MEMORY(tmp, "Hello", 5);
    ASSERT_EQ(t2->available, 0);

    rc = ks_ringbuffer_write(rb, "Hello", 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 2);

    ASSERT_MEMORY(rb->bfr, "lolloHel", 8);
    ASSERT_EQ(t->tail_index, 2);
    ASSERT_EQ(t->truncated_bytes, 2);

    rc = ks_ringbuffer_read(rb, t, tmp, 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_MEMORY(tmp, "lloHello", 8);
    ASSERT_EQ(rb->head_index, 2);
    ASSERT_EQ(t->tail_index, 2);

    rc = ks_ringbuffer_read(rb, t2, tmp, 5);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_MEMORY(tmp, "Hello", 5);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);

}


TEST(rb_test_write_too_much)
{
    struct ks_ringbuffer *rb = NULL;
    int rc;

    rc = ks_ringbuffer_init(&rb, 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "01234567", 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 0);

    rc = ks_ringbuffer_write(rb, "012345670", 9);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);
}


TEST(rb_test_read_too_much)
{
    struct ks_ringbuffer *rb = NULL;
    struct ks_ringbuffer_tail *t = NULL;
    char tmp[16];
    int rc;

    rc = ks_ringbuffer_init(&rb, 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_write(rb, "01234567", 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(rb->head_index, 0);

    rc = ks_ringbuffer_read(rb, t, tmp, 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(t->tail_index, 0);
    ASSERT_EQ(rb->head_index, 0);
    ASSERT_MEMORY(tmp, "01234567", 8);

    rc = ks_ringbuffer_read(rb, t, tmp, 1);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ringbuffer_read(rb, t, tmp, 32);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ringbuffer_write(rb, "01234567", 8);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_read(rb, t, tmp, 8);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_MEMORY(tmp, "01234567", 8);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);
}

uint64_t advance_tail(struct ks_ringbuffer *rb, struct ks_ringbuffer_tail *t,
                      uint64_t new_index)
{
    int rc;
    uint32_t *payload_sz = NULL; 
    uint64_t current_index = t->tail_index;

    while (current_index < new_index)
    {
        payload_sz = (uint32_t *) &rb->bfr[current_index];
        current_index = (current_index+sizeof(uint32_t)+*payload_sz)%rb->bfr_sz;
    }
    
    return current_index;
}

TEST(rb_write_objs)
{
    int rc;
    struct ks_ringbuffer *rb;
    struct ks_ringbuffer_tail *t;
    char tmp[16];

    rc = ks_ringbuffer_init(&rb, 16);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ringbuffer_new_tail(rb, &t);
    ASSERT_EQ(rc, KS_OK);
    
    rc = ks_ringbuffer_set_tail_advance_cb(t, advance_tail);
    ASSERT_EQ(rc, KS_OK);


    /* 0123456789ABCDEF 16 byte buffer*/
    /* t0               Tail at pos 0 */
    /* (Obj1 )(Obj2 )   Two objects written in buffer */
    /* bj3 )  (Obj2 )(O Third object is written  */
    /*        t0        Since there is not room for more than
     *                  Two objects in the buffer
     *                  Object 1 must be removed and tail t0 advanced
     *                  to point at the beginning of object 2  */

    uint32_t payload_length = 3;
    const char *payload = "Hej";
    const char *payload2 = "He2";
    const char *payload3 = "He3";

    /* Write three objects */
    rc = ks_ringbuffer_write(rb, &payload_length, 4);
    ASSERT_EQ(rc, KS_OK); 
    rc = ks_ringbuffer_write(rb, payload, 3);
    ASSERT_EQ(rc, KS_OK); 

    rc = ks_ringbuffer_write(rb, &payload_length, 4);
    ASSERT_EQ(rc, KS_OK); 
    rc = ks_ringbuffer_write(rb, payload2, 3);
    ASSERT_EQ(rc, KS_OK); 


    ASSERT_EQ(t->tail_index, 0);
    ASSERT_EQ(t->truncated_bytes, 0);

    rc = ks_ringbuffer_write(rb, &payload_length, 4);
    ASSERT_EQ(rc, KS_OK); 
    rc = ks_ringbuffer_write(rb, payload3, 3);
    ASSERT_EQ(rc, KS_OK); 

    ASSERT_EQ(t->tail_index, 7);
    ASSERT_EQ(t->truncated_bytes, 7);

    rc = ks_ringbuffer_read(rb, t, tmp, 7);
    ASSERT_EQ(rc, KS_OK);
    uint32_t *readback_sz = (uint32_t *) tmp;
    ASSERT_EQ(*readback_sz, 3);
    ASSERT_MEMORY(&tmp[4], "He2", 3);
    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 14);

    rc = ks_ringbuffer_read(rb, t, tmp, 7);
    ASSERT_EQ(rc, KS_OK);
    readback_sz = (uint32_t *) tmp;
    ASSERT_EQ(*readback_sz, 3);
    ASSERT_MEMORY(&tmp[4], "He3", 3);

    ASSERT_EQ(rb->head_index, 5);
    ASSERT_EQ(t->tail_index, 5);

    rc = ks_ringbuffer_free(rb);
    ASSERT_EQ(rc, KS_OK);
}
