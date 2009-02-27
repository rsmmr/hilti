/* $Id$
 * 
 * Support functions HILTI's string data type.
 * 
 */

#include "hilti_intern.h"
#include "utf8proc.h"

__hlt_string_size_t __hlt_string_len(const __hlt_string* s)
{
    int32_t dummy;
    int8_t* p = s->bytes;
    int8_t* e = p + s->len;
    __hlt_string_size_t len = 0; 
    
    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        if ( n < 0 )
            __hlt_exception_raise(__hlt_exception_value_error);

        ++len; 
        p += n;
    }

    if ( p != e )
        __hlt_exception_raise(__hlt_exception_value_error);

    return len;
}

const __hlt_string* __hlt_string_concat(const __hlt_string* s1, const __hlt_string* s2)
{
    return 0;
}

const __hlt_string* __hlt_string_substr(const __hlt_string* s1, __hlt_string_size_t pos, __hlt_string_size_t len)
{
    return 0;
}

__hlt_string_size_t __hlt_string_find(const __hlt_string* s, const __hlt_string* pattern)
{
    return 0;
}

int __hlt_string_cmp(const __hlt_string* s1, const __hlt_string* s2)
{
    return 0;
}
