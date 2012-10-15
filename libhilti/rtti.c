
#include <assert.h>
#include <string.h>

#include "rtti.h"

int8_t hlt_type_equal(const hlt_type_info* ti1, const hlt_type_info* ti2)
{
    assert(ti1);
    assert(ti2);
    return ti1->type == ti2->type && strcmp(ti1->tag, ti2->tag) == 0;
}
