/* $Id$
 * 
 * Support functions HILTI's double data type.
 * 
 */

#include <stdio.h>
#include <string.h>

#include "hilti_intern.h"

const __hlt_string* __hlt_double_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* exception)
{
    assert(type->type == __HLT_TYPE_DOUBLE);
    
    double val = *((double *)obj);

    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own itoa().
    char buffer[128];
    int len = snprintf(buffer, 128, "%.2f", val);
    __hlt_string *s = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len);
    memcpy(s->bytes, buffer, len);
    s->len = len;
    return s;
}

double __hlt_double_to_double(const __hlt_type_info* type, const void* obj, __hlt_exception* expt)
{
    assert(type->type == __HLT_TYPE_DOUBLE);
    double val = *((double *)obj);
    return val;
}
