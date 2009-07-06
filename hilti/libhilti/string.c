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

static __hlt_string_constant EmptyString = { 0, "" };

__hlt_string __hlt_string_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    __hlt_string s = *((__hlt_string*)obj);
    return s ? s : &EmptyString;
}

__hlt_string_size __hlt_string_len(__hlt_string s, __hlt_exception* excpt)
{
    int32_t dummy;
    const int8_t* p;
    const int8_t* e;
    
    if ( ! s )
        return 0;
    
    p = s->bytes;
    e = p + s->len;

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

__hlt_string __hlt_string_concat(__hlt_string s1, __hlt_string s2, __hlt_exception* excpt)
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
    
    __hlt_string dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len1 + len2);
    
    if ( ! dst ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }
    
    dst->len = len1 + len2;
    memcpy(dst->bytes, s1->bytes, len1);
    memcpy(dst->bytes + len1, s2->bytes, len2);

    return dst;
}

__hlt_string __hlt_string_substr(__hlt_string s1, __hlt_string_size pos, __hlt_string_size len, __hlt_exception* excpt)
{
    if ( ! s1 )
        s1 = &EmptyString;
        
    return 0;
}

__hlt_string_size __hlt_string_find(__hlt_string s, __hlt_string pattern, __hlt_exception* excpt)
{
    if ( ! s )
        s = &EmptyString;
        
    if ( ! pattern )
        pattern = &EmptyString;

    return 0;
}

int8_t __hlt_string_cmp(__hlt_string s1, __hlt_string s2, __hlt_exception* excpt)
{
    const int8_t* p1;
    const int8_t* p2;
    
    if ( ! s1 )
        s1 = &EmptyString;
        
    if ( ! s2 )
        s2 = &EmptyString;

    if ( s1->len == 0 && s2->len == 0 )
        return 0;
    
    if ( s1->len == 0 )
        return -1;
    
    if ( s2->len == 0 )
        return 1;
        
    p1 = s1->bytes;
    p2 = s2->bytes;
    
    __hlt_string_size i = 0; 

    while ( true ) {
        ssize_t n1, n2;
        int32_t cp1, cp2;
        
        n1 = utf8proc_iterate((uint8_t *)p1, s1->len - i, &cp1);
        if ( n1 < 0 ) {
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        n2 = utf8proc_iterate((uint8_t *)p2, s2->len - i, &cp2);
        if ( n2 < 0 ) {
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        if ( cp1 != cp2 )
            return (int8_t)(cp1 - cp2);
        
        if ( i == s1->len || i == s2->len )
            break;
        
        p1 += n1;
        p2 += n2;
        i++;
    }
    
    if ( i == s1->len && i == s2->len )
        return 0;
    
    if ( i == s1->len )
        return -1;
        
    if ( i == s2->len )
        return 1;
    
    return 0;
}

__hlt_string __hlt_string_from_asciiz(const char* asciiz, __hlt_exception* excpt)
{
    __hlt_string_size len = strlen(asciiz);
    __hlt_string dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len);
    dst->len = len;
    memcpy(dst->bytes, asciiz, len);
    return dst;
}

__hlt_string __hlt_string_from_data(const int8_t* data, __hlt_string_size len, __hlt_exception* excpt)
{
    __hlt_string dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + len);
    dst->len = len;
    memcpy(dst->bytes, data, len);
    return dst;
}

static __hlt_string_constant UTF8_STRING = { 4, "utf8" };
static __hlt_string_constant ASCII_STRING = { 5, "ascii" };
enum Charset { ERROR, UTF8, ASCII };

static enum Charset get_charset(__hlt_string charset, __hlt_exception* excpt)
{
    if ( __hlt_string_cmp(charset, &UTF8_STRING, excpt) == 0 )
        return UTF8;
    
    if ( __hlt_string_cmp(charset, &ASCII_STRING, excpt) == 0 )
        return ASCII;
    
    *excpt = __hlt_exception_value_error;
    return ERROR;
}

__hlt_bytes* __hlt_string_encode(__hlt_string s, __hlt_string charset, __hlt_exception* excpt)
{
    const int8_t* p;
    const int8_t* e;
    __hlt_bytes* dst = __hlt_bytes_new(excpt);
    
    enum Charset ch = get_charset(charset, excpt);
    if ( ch == ERROR )
        return 0;
    
    if ( ! s )
        return dst;

    p = s->bytes;
    e = p + s->len;

    if ( ch == UTF8 ) {
        // Caller wants UTF-8 so we just need append it literally. 
        __hlt_bytes_append_raw(dst, s->bytes, s->len, excpt);
        return dst;
    }
    
    while ( p < e ) {
        
        int32_t uc;
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &uc);
        
        if ( n < 0 ) {
            *excpt = __hlt_exception_value_error;
            return 0;
        }
        
        switch ( ch ) {
          case ASCII: {
              // Replace non-ASCII characters with '?'.
              int8_t c = uc < 128 ? (int8_t)uc : '?';
              __hlt_bytes_append_raw(dst, &c, 1, excpt);
              break;
          }
            
          default:
            assert(false);
        }
        
        p += n;
    }
    
    return dst;
}

__hlt_string __hlt_string_decode(__hlt_bytes* b, __hlt_string charset, __hlt_exception* excpt)
{
    enum Charset ch = get_charset(charset, excpt);
    if ( ch == ERROR )
        return 0;
    
    if ( __hlt_bytes_empty(b, excpt) )
        return &EmptyString;
        
    if ( ch == UTF8 ) {
        // Data is already in UTF-8, just need to copy it into a string.
        __hlt_bytes_pos begin = __hlt_bytes_begin(b, excpt);
        const __hlt_bytes_pos end = __hlt_bytes_end(excpt);
        const int8_t* raw = __hlt_bytes_sub_raw(begin, end, excpt);
        const __hlt_string dst = __hlt_string_from_data(raw, __hlt_bytes_len(b, excpt), excpt);
        return dst;
    }
    
    if ( ch == ASCII ) {
        // Convert all bytes to 7-bit codepoints.
        __hlt_bytes_size len = __hlt_bytes_len(b, excpt);
        __hlt_string dst = __hlt_gc_malloc_atomic(sizeof(__hlt_string) + __hlt_bytes_len(b, excpt));
        dst->len = len;
        int8_t* p = dst->bytes;
        
        __hlt_bytes_pos i = __hlt_bytes_begin(b, excpt);
        while ( ! __hlt_bytes_pos_eq(i, __hlt_bytes_end(excpt), excpt) ) {
            char c = __hlt_bytes_pos_deref(i, excpt);
            *p++ = (c & 0x7f) == c ? c : '?';
            i = __hlt_bytes_pos_incr(i, excpt);
        }
        
        return dst;
    }

    assert(false); // cannot be reached.
    return 0;
}
