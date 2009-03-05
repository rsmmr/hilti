/* $Id$
 * 
 * Support functions HILTI's integer data type.
 * 
 */

#include <stdio.h>
#include <string.h>

#include "hilti_intern.h"

const struct __hlt_string* __hlt_int_fmt(int64_t val, int32_t options, __hlt_exception* exception)
{
    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own itoa().
    char buffer[128];
    int len = snprintf(buffer, 128, "%lld", val);
    struct __hlt_string *s = __hlt_gc_malloc_atomic(sizeof(struct __hlt_string) + len);
    memcpy(s->bytes, buffer, len);
    s->len = len;
    return s;
}

