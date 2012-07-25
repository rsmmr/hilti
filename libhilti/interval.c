//
// Support functions HILTI's interval data type.
//

#include <stdio.h>
#include <string.h>

#include "interval.h"
#include "string_.h"

extern const hlt_type_info hlt_type_info_double;

hlt_string hlt_interval_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);

    uint64_t secs = val / 1000000000;
    double frac = (val % 1000000000) / 1e9;

    char buffer[128];
    snprintf(buffer, 128, "%.6fs", ( (double)secs + ((double)frac)) );
    return hlt_string_from_asciiz(buffer, excpt, ctx);
}

double hlt_interval_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);

    return val / 1e9;;
}

int64_t hlt_interval_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTERVAL);
    hlt_interval val = *((hlt_interval *)obj);
    return (int64_t)(val / 1e9);
}

uint64_t hlt_interval_nsecs(hlt_interval t, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return t;
}

