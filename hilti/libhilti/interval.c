/* $Id$
 *
 * Support functions HILTI's time data type.
 *
 */

#include "hilti.h"

#include <stdio.h>
#include <string.h>

hlt_string hlt_interval_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);

    uint64_t secs = (val >> 32);
    uint64_t nanos = val & 0xffffffff;

    char buffer[128];
    int len = snprintf(buffer, 128, "%.fs", ( (double)secs + ((double)nanos / 1e9)) );
    hlt_string s = hlt_gc_malloc_atomic(sizeof(hlt_string) + len);
    memcpy(s->bytes, buffer, len);
    s->len = len;
    return s;
}

double hlt_interval_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);

    uint64_t secs = (val >> 32);
    uint64_t nanos = val & 0xffffffff;

    return (double)secs + (nanos / 1e9);
}

int64_t hlt_interval_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    // This returns nanoseconds.
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);

    return val;
}
