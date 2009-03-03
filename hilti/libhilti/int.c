/* $Id$
 * 
 * Support functions HILTI's integer data type.
 * 
 */

#include "hilti_intern.h"

const struct __hlt_string* __hlt_int_fmt(int32_t val, int32_t options, __hlt_exception_t* exception)
{
    int len = 0;
    struct __hlt_string *s = __hlt_gc_malloc_atomic(sizeof(struct __hlt_string) + len);
    s->len = len;
    return s;
}

