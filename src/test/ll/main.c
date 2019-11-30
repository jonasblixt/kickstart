#include <stdio.h>
#include <nala.h>
#include <unistd.h>
#include <kickstart/ll.h>

TEST(ll_init)
{
    int rc;
    struct ks_ll *ll;

    rc = ks_ll_init(&ll);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_ERR);
}

TEST(ll_init_null)
{
    int rc;

    rc = ks_ll_init(NULL);
    ASSERT_EQ(rc, KS_ERR);
}

TEST(ll_free_one)
{
    int rc;
    struct ks_ll *ll;

    rc = ks_ll_init(&ll);
    ASSERT_EQ(rc, KS_OK);

    int i = 0x1234;
    rc = ks_ll_append(ll, &i);
    ASSERT_EQ(rc, KS_OK);
    
    rc = ks_ll_append(ll, NULL);
    ASSERT_EQ(rc, KS_ERR);

    ks_ll_foreach(ll, item)
    {
        int *p = item->data;
        ASSERT_EQ(*p, 0x1234);
    }

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_OK);
}

TEST(ll_free_two)
{
    int rc;
    struct ks_ll *ll;

    rc = ks_ll_init(&ll);
    ASSERT_EQ(rc, KS_OK);

    int i = 0x1234;
    rc = ks_ll_append(ll, &i);
    ASSERT_EQ(rc, KS_OK);
    
    rc = ks_ll_append(ll, NULL);
    ASSERT_EQ(rc, KS_ERR);

    ks_ll_foreach(ll, item)
    {
        int *p = item->data;
        ASSERT_EQ(*p, 0x1234);
    }

    rc = ks_ll_remove(ll, ll->start);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ll_remove(ll, ll->start);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_OK);

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_ERR);
}

TEST(ll_append2)
{
    int rc;
    struct ks_ll *ll;

    rc = ks_ll_init(&ll);
    ASSERT_EQ(rc, KS_OK);

    int *i;
    rc = ks_ll_append2(ll, (void **) &i, sizeof(*i));
    ASSERT_EQ(rc, KS_OK);
    *i = 0x1234;

    rc = ks_ll_append(ll, NULL);
    ASSERT_EQ(rc, KS_ERR);

    ks_ll_foreach(ll, item)
    {
        int *p = item->data;
        ASSERT_EQ(*p, 0x1234);
    }

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_OK);
}


TEST(ll_pop)
{
    int rc;
    struct ks_ll *ll;

    rc = ks_ll_init(&ll);
    ASSERT_EQ(rc, KS_OK);

    int *i;
    rc = ks_ll_append2(ll, (void **) &i, sizeof(*i));
    ASSERT_EQ(rc, KS_OK);
    *i = 0x1234;

    rc = ks_ll_append2(ll, (void **) &i, sizeof(*i));
    ASSERT_EQ(rc, KS_OK);
    *i = 0x1235;

    int *p;
    rc = ks_ll_pop(ll, (void **) &p);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(*p, 0x1235);
    free(p);

    rc = ks_ll_pop(ll, (void **) &p);
    ASSERT_EQ(rc, KS_OK);
    ASSERT_EQ(*p, 0x1234);
    free(p);

    rc = ks_ll_pop(ll, (void **) &p);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_free(&ll);
    ASSERT_EQ(rc, KS_OK);
}

TEST(ll_null_args)
{
    int rc;
    int p;

    rc = ks_ll_data2item(NULL, &p, (struct ks_ll_item **) &p);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_data2item((struct ks_ll *) &p, NULL, (struct ks_ll_item **) &p);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_data2item((struct ks_ll *) &p, &p, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_pop(NULL, (void **) &p);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_pop((struct ks_ll *) &p, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_remove(NULL, NULL);
    ASSERT_EQ(rc, KS_ERR);

    rc = ks_ll_free(NULL);
    ASSERT_EQ(rc, KS_ERR);
}
