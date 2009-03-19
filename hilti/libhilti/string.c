/* $Id$
 * 
 * Support functions HILTI's string data type.
 * 
 * Note that we treat a null pointer as a legal representation of the empty
 * string. That simplifies its handling as a constant. 
 */

#include <string.h>

#include "hilti_intern.h"
#include "utf8proc.h"

static const __hlt_string EmptyString = { 0, "" };

const __hlt_string* __hlt_string_to_string(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* excpt)
{
    __hlt_string* s = *((__hlt_string**)obj);
    return s ? s : &EmptyString;
}

__hlt_string_size __hlt_string_len(const __hlt_string* s, __hlt_exception* excpt)
{
    int32_t dummy;
    const int8_t* p = s->bytes;
    const int8_t* e = p + s->len;
    __hlt_string_size len = 0; 

    if ( ! s )
        return 0;
    
    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        if ( n < 0 ) {
        *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        ++len; 
        p += n;
    }
    
    if ( p != e ) {
        *excpt = __hlt_exception_value_error;
        return 0;
    }
    
    return len;
}

#include <stdio.h>

const __hlt_string* __hlt_string_concat(const __hlt_string* s1, const __hlt_string* s2, __hlt_exception* excpt)
{
    __hlt_string_size len1;
    __hlt_string_size len2;; 

    if ( ! s1 )
        s1 = &EmptyString;
        
    if ( ! s2 )
        s2 = &EmptyString;

    len1 = s1->len; 
    len2 = s2->len; 
        
    if ( ! len1 )
        return s2;
    
    if ( ! len2 )
        return s1;
    
    __hlt_string *dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len1 + len2);
    
    if ( ! dst ) {
        *excpt = __hlt_exception_value_error;
        return 0;
    }
    
    dst->len = len1 + len2;
    memcpy(dst->bytes, s1->bytes, len1);
    memcpy(dst->bytes + len1, s2->bytes, len2);

    return dst;
}

const __hlt_string* __hlt_string_substr(const __hlt_string* s1, __hlt_string_size pos, __hlt_string_size len, __hlt_exception* excpt)
{
    if ( ! s1 )
        s1 = &EmptyString;
        
    return 0;
}

__hlt_string_size __hlt_string_find(const __hlt_string* s, const __hlt_string* pattern, __hlt_exception* excpt)
{
    if ( ! s )
        s = &EmptyString;
        
    if ( ! pattern )
        pattern = &EmptyString;

    return 0;
}

int __hlt_string_cmp(const __hlt_string* s1, const __hlt_string* s2, __hlt_exception* excpt)
{
    if ( ! s1 )
        s1 = &EmptyString;
        
    if ( ! s2 )
        s2 = &EmptyString;
        
    return 0;
}

const __hlt_string* __hlt_string_from_asciiz(const char* asciiz, __hlt_exception* excpt)
{
    __hlt_string_size len = strlen(asciiz);
    __hlt_string *dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len);
    dst->len = len;
    memcpy(dst->bytes, asciiz, len);
    return dst;
}

const __hlt_string* __hlt_string_from_data(const int8_t* data, __hlt_string_size len, __hlt_exception* excpt)
{
    __hlt_string *dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len);
    dst->len = len;
    memcpy(dst->bytes, data, len);
    return dst;
}

