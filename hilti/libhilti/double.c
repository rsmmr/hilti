/* $Id$
 * 
 * Support functions HILTI's double data type.
 * 
 */

#include <stdio.h>
#include <string.h>

#include "hilti.h"

hlt_string hlt_double_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception)
{
    assert(type->type == HLT_TYPE_DOUBLE);
    
    double val = *((double *)obj);

    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own itoa().
    char buffer[128];
    int len = snprintf(buffer, 128, "%.2f", val);
    hlt_string s = hlt_gc_malloc_atomic(sizeof(hlt_string) + len);
    memcpy(s->bytes, buffer, len);
    s->len = len;
    return s;
}

double hlt_double_to_double(const hlt_type_info* type, const void* obj, hlt_exception** expt)
{
    assert(type->type == HLT_TYPE_DOUBLE);
    double val = *((double *)obj);
    return val;
}
