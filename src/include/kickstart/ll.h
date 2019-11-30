#ifndef INCLUDE_KICKSTART_LL_H_
#define INCLUDE_KICKSTART_LL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kickstart/kickstart.h>

struct ks_ll_item
{
    void *data;
    bool needs_free;
    struct ks_ll_item *prev;
    struct ks_ll_item *next;
};

struct ks_ll
{
    struct ks_ll_item *start;
    struct ks_ll_item *end;
};

#define ks_ll_foreach(__ll, __name) \
    for (struct ks_ll_item *__name = __ll->start; __name; __name = __name->next)

int ks_ll_init(struct ks_ll **ll);
int ks_ll_free(struct ks_ll **ll);
int ks_ll_append(struct ks_ll *ll, const void *data);
int ks_ll_append2(struct ks_ll *ll, void **data, size_t sz);
int ks_ll_remove(struct ks_ll *ll, struct ks_ll_item *item);
int ks_ll_pop(struct ks_ll *ll, void **data);
int ks_ll_data2item(struct ks_ll *ll, void *data, struct ks_ll_item **item);

#endif  // INCLUDE_KICKSTART_LL_H_
