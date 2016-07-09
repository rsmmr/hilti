
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rtti.h"

int8_t __hlt_type_equal(const hlt_type_info* ti1, const hlt_type_info* ti2)
{
    if ( ! ti1 )
        return (ti2 == 0);

    if ( ! ti2 )
        return (ti1 == 0);

    // The pointer check isn't sufficient as we may have two copies of the TI
    // in JIT mode. It's a good short-cut, though.
    return (ti1 == ti2) || (ti1->type == ti2->type && strcmp(ti1->tag, ti2->tag) == 0);
}
