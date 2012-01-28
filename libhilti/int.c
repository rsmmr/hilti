/* $Id$
 *
 * Support functions HILTI's integer data type.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "int.h"
#include "rtti.h"
#include "string_.h"

// Converts the integer into a int64_t correctly considering its width and
// assuming a signed value.
static int64_t _makeInt64Signed(const hlt_type_info* type, const void *obj)
{
    // The first (and only) type parameter is an int64_t with the wdith.
    int64_t *width = (int64_t*) &(type->type_params);
    int64_t val;

    if ( *width <= 8 )
        val = *((int8_t *)obj);
    else if ( *width <= 16 )
        val = *((int16_t *)obj);
    else if ( *width <= 32 )
        val = *((int32_t *)obj);
    else {
        assert(*width <= 64);
        val = *((int64_t *)obj);
    }

    return val;
}

// Converts the integer into a int64_t correctly considering its width and
// assuming an usigned value.
static uint64_t _makeInt64Unsigned(const hlt_type_info* type, const void *obj)
{
    // The first (and only) type parameter is an int64_t with the wdith.
    int64_t *width = (int64_t*) &(type->type_params);
    uint64_t val;

    if ( *width <= 8 )
        val = *((uint8_t *)obj);
    else if ( *width <= 16 )
        val = *((uint16_t *)obj);
    else if ( *width <= 32 )
        val = *((uint32_t *)obj);
    else {
        assert(*width <= 64);
        val = *((uint64_t *)obj);
    }

    return val;
}

uint64_t hlt_int_pow(uint64_t base, uint64_t exp, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int result = 1;

    while ( exp ) {
        if ( exp % 2 )
            result *= base;

        exp >>= 1;
        base *= base;
    }

    return result;
}

hlt_string hlt_int_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTEGER);

    static const char *fmt_signed = "%" PRId64;
    static const char *fmt_unsigned = "%" PRId64;

    const char *fmt;
    int64_t val;

    if ( options & HLT_CONVERT_UNSIGNED ) {
        fmt = fmt_unsigned;
        val = _makeInt64Unsigned(type, obj);
    }

    else {
        fmt = fmt_signed;
        val = (int64_t)_makeInt64Signed(type, obj);
    }

    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own itoa().
    char buffer[128];
    int len = snprintf(buffer, 128, fmt, val);
    hlt_string s = hlt_string_from_data((int8_t*)buffer, len, exception, ctx);
    return s;
}

int64_t hlt_int_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTEGER);

    if ( options & HLT_CONVERT_UNSIGNED )
        return (int64_t)_makeInt64Unsigned(type, obj);
    else
        return (int64_t)_makeInt64Signed(type, obj);
}

