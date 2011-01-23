/* $Id$
 *
 * Support functions HILTI's time data type.
 *
 */

#include "hilti.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static hlt_string_constant not_set = HLT_STRING_CONSTANT("<time not set>");

hlt_time hlt_time_value(uint64_t secs, uint64_t nsecs)
{
    return (secs << 32) | nsecs;
}

extern const hlt_type_info hlt_type_info_double;

hlt_string hlt_time_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIME);
    hlt_time val = *((hlt_time *)obj);

    if ( val == HLT_TIME_UNSET )
        return &not_set;

    time_t secs = (time_t)(val >> 32);

    char buffer[30];
    char buffer2[30];

    snprintf(buffer, sizeof(buffer2), ".%.9fZ", (val & 0xffffffff) / 1e9);

    struct tm tm;
    size_t len = strftime(buffer2, sizeof(buffer2), "%Y-%m-%dT%H:%M:%S", gmtime_r(&secs, &tm));

    hlt_string s = hlt_string_from_asciiz(buffer2, excpt, ctx);
    s = hlt_string_concat(s, hlt_string_from_asciiz(buffer + 2, excpt, ctx), excpt, ctx);
    return s;
}

double hlt_time_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TIME);
    hlt_time val = *((hlt_time *)obj);

    if ( val == HLT_TIME_UNSET )
        return -1;

    uint64_t secs = (val >> 32);
    uint64_t nanos = val & 0xffffffff;

    return (double)secs + (nanos / 1e9);
}

int64_t hlt_time_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    // This prints nanoseconds.
    assert(type->type == HLT_TYPE_TIME);
    hlt_time val = *((hlt_time *)obj);
    return val;
}

hlt_time hlt_time_wall(hlt_exception** excpt, hlt_execution_context* ctx)
{
    struct timeval tv;

    if ( gettimeofday(&tv, NULL) != 0 ) {
        // Don't think this can ever fail ...
        hlt_set_exception(excpt, &hlt_exception_internal_error, 0);
        return 0;
    }

    uint64_t secs = tv.tv_sec;
    uint64_t nanos = tv.tv_usec * 1000;
    return (secs << 32) | nanos;
}
