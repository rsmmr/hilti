/* $Id$
 * 
 * Support functions HILTI's integer data type.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hilti_intern.h"

// Converts the integer into a int64_t correctly considering its width.
static int64_t _makeInt64(const __hlt_type_info* type, void *obj)
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

const struct __hlt_string* __hlt_int_fmt(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* exception)
{
    int64_t val = _makeInt64(type, obj);
    
    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own itoa().
    char buffer[128];
    int len = snprintf(buffer, 128, "%lld", val);
    struct __hlt_string *s = __hlt_gc_malloc_atomic(sizeof(struct __hlt_string) + len);
    memcpy(s->bytes, buffer, len);
    s->len = len;
    return s;
}

