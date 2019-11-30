#include <kickstart/ll.h>
#include <stdlib.h>
#include <string.h>

int ks_ll_init(struct ks_ll **ll)
{
    if (!ll)
        return KS_ERR;

    (*ll) = malloc(sizeof(struct ks_ll));

    if (!(*ll))
        return KS_ERR;

    memset((*ll), 0, sizeof(struct ks_ll));

    return KS_OK;
}

int ks_ll_free(struct ks_ll **ll)
{
    if (!ll)
        return KS_ERR;
    if (!(*ll))
        return KS_ERR;

    if ((*ll)->start)
    {
        struct ks_ll_item *p = (*ll)->start;

        while (p)
        {
            struct ks_ll_item *next = p->next;

            if (p->needs_free)
                free(p->data);

            free(p);
            p = next;
        }

    }

    free(*ll);
    *ll = NULL;
    return KS_OK;
}

static int __ks_ll_append(struct ks_ll *ll, const void *data, bool needs_free)
{
    if (!ll)
        return KS_ERR;
    if (!data)
        return KS_ERR;

    struct ks_ll_item *new = malloc(sizeof(struct ks_ll_item));

    if (!new)
        return KS_ERR;

    memset(new, 0, sizeof(struct ks_ll_item));
    new->data = (void *) data;
    new->needs_free = needs_free;

    if (!ll->start)
    {
        ll->start = new;
        ll->end = new;
    }
    else
    {
        if (!ll->end)
            return KS_ERR;

        ll->end->next = new;
        new->prev = ll->end;
        ll->end = new;
    }

    return KS_OK;
}

int ks_ll_append(struct ks_ll *ll, const void *data)
{
    return __ks_ll_append(ll, data, false);
}


int ks_ll_append2(struct ks_ll *ll, void **data, size_t sz)
{
    int rc;

    if (!ll)
        return KS_ERR;
    if (!data)
        return KS_ERR;

    void *p = malloc(sz);

    if (!p)
        return KS_ERR;

    memset(p, 0 , sz);
    rc = __ks_ll_append(ll, p, true);

    if (rc != KS_OK)
    {
        free(p);
        *data = NULL;
    }
    *data = p;
    return rc;
}

static int __ks_ll_remove(struct ks_ll *ll, struct ks_ll_item *item,
                          bool data_free)
{
    if (!ll)
        return KS_ERR;
    if (!item)
        return KS_ERR;

    if (item->prev)
        item->prev->next = item->next;

    if (item->next)
        item->next->prev = item->prev;

    if (item->needs_free && data_free)
    {
        free(item->data);
        item->data = NULL;
    }

    if (item == ll->start)
        ll->start = item->next;
    if (item == ll->end)
        ll->end = item->prev;

    free (item);

    return KS_OK;
}

int ks_ll_remove(struct ks_ll *ll, struct ks_ll_item *item)
{
    return __ks_ll_remove(ll, item, true);
}

int ks_ll_pop(struct ks_ll *ll, void **data)
{
    if (!ll)
        return KS_ERR;
    if (!data)
        return KS_ERR;
    if (!ll->end)
        return KS_ERR;

    *data = ll->end->data;

    return __ks_ll_remove(ll, ll->end, false);
}

int ks_ll_data2item(struct ks_ll *ll, void *data, struct ks_ll_item **item)
{
    if (!ll)
        return KS_ERR;
    if (!data)
        return KS_ERR;
    if (!item)
        return KS_ERR;
    *item = NULL;
    ks_ll_foreach(ll, i)
    {
        if (i->data == data)
            *item = i;
    }
    return (*item != NULL)?KS_OK:KS_ERR;
}
