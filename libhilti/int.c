/* $Id$
 *
 * Support functions HILTI's integer data type.
 *
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "autogen/hilti-hlt.h"
#include "int.h"
#include "rtti.h"
#include "string_.h"

// Converts the integer into a int64_t correctly considering its width and
// assuming a signed value.
static int64_t _makeInt64Signed(const hlt_type_info* type, const void* obj)
{
    // The first (and only) type parameter is an int64_t with the wdith.
    int64_t* width = (int64_t*)&(type->type_params);
    int64_t val;

    if ( *width <= 8 )
        val = *((int8_t*)obj);
    else if ( *width <= 16 )
        val = *((int16_t*)obj);
    else if ( *width <= 32 )
        val = *((int32_t*)obj);
    else {
        assert(*width <= 64);
        val = *((int64_t*)obj);
    }

    return val;
}

// Converts the integer into a int64_t correctly considering its width and
// assuming an usigned value.
static uint64_t _makeInt64Unsigned(const hlt_type_info* type, const void* obj)
{
    // The first (and only) type parameter is an int64_t with the wdith.
    int64_t* width = (int64_t*)&(type->type_params);
    uint64_t val;

    if ( *width <= 8 )
        val = *((uint8_t*)obj);
    else if ( *width <= 16 )
        val = *((uint16_t*)obj);
    else if ( *width <= 32 )
        val = *((uint32_t*)obj);
    else {
        assert(*width <= 64);
        val = *((uint64_t*)obj);
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

hlt_string hlt_int_to_string(const hlt_type_info* type, const void* obj, int32_t options,
                             __hlt_pointer_stack* seen, hlt_exception** exception,
                             hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTEGER);

    static const char* fmt_signed = "%" PRId64;
    static const char* fmt_unsigned = "%" PRId64;

    const char* fmt;
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
    return hlt_string_from_data((int8_t*)buffer, len, exception, ctx);
}

int64_t hlt_int_to_int64(const hlt_type_info* type, const void* obj, int32_t options,
                         hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_INTEGER);

    if ( options & HLT_CONVERT_UNSIGNED )
        return (int64_t)_makeInt64Unsigned(type, obj);
    else
        return (int64_t)_makeInt64Signed(type, obj);
}

int64_t hlt_int_to_host(int64_t v, hlt_enum byte_order, int64_t n, hlt_exception** excpt,
                        hlt_execution_context* ctx)
{
    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Host, excpt, ctx) )
        return v;

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Big, excpt, ctx) )
        return (hlt_ntoh64(v) >> (64 - n * 8));

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Little, excpt, ctx) )
#if __BIG_ENDIAN__
        return (hlt_flip64(v) >> (64 - n * 8));
#else
        return v;
#endif

    hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
    return 0;
}

int64_t hlt_int_from_host(int64_t v, hlt_enum byte_order, int64_t n, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Host, excpt, ctx) )
        return v;

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Big, excpt, ctx) )
        return (hlt_ntoh64(v) >> (64 - n * 8));

    if ( hlt_enum_equal(byte_order, Hilti_ByteOrder_Little, excpt, ctx) )
#if __BIG_ENDIAN__
        return (hlt_flip64(v) >> (64 - n * 8));
#else
        return v;
#endif

    hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
    return 0;
}

int64_t hlt_int_flip(int64_t v, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return (hlt_flip64(v) >> (64 - n * 8));
}

int64_t hlt_int_width(const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( type->type != HLT_TYPE_INTEGER ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    int64_t* width = (int64_t*)&(type->type_params);
    return *width;
}
