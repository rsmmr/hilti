/* $Id$
 * 
 * Support functions HILTI's string data type.
 * 
 */

#include <string.h>

#include "hilti_intern.h"
#include "utf8proc.h"

const __hlt_string* __hlt_string_fmt(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* excpt)
{
    return *((__hlt_string**)obj);
}

__hlt_string_size __hlt_string_len(const __hlt_string* s, __hlt_exception* excpt)
{
    int32_t dummy;
    const int8_t* p = s->bytes;
    const int8_t* e = p + s->len;
    __hlt_string_size len = 0; 
    
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
    __hlt_string_size len1 = s1->len; 
    __hlt_string_size len2 = s2->len; 

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
    return 0;
}

__hlt_string_size __hlt_string_find(const __hlt_string* s, const __hlt_string* pattern, __hlt_exception* excpt)
{
    return 0;
}

int __hlt_string_cmp(const __hlt_string* s1, const __hlt_string* s2, __hlt_exception* excpt)
{
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

static const int BufferSize = 128;


static void _add_char(int8_t c, int8_t* buffer, __hlt_string_size* bpos, const __hlt_string** dst, __hlt_exception* excpt)
{ 
    // Adds one character 'c' to the string we're building. If there's space
    // in the buffer, it's added there and 'dst' is returned unmodified. If
    // not, the current buffer is added to 'dst', the buffer is cleared and
    // the character is inserted into it. 
    if ( *bpos < BufferSize - 1 ) 
        // We have space.
        buffer[(*bpos)++] = c;
    
    else {
        // Copy buffer to destination string and start over.
        const __hlt_string* new_dst = __hlt_string_from_data(buffer, *bpos, excpt);
        if ( *excpt )
            return;
            
        new_dst = __hlt_string_concat(*dst, new_dst, excpt);
        buffer[0] = c;
        *bpos = 1;
        *dst = new_dst;
    }
}

static void _add_chars(const int8_t* data, __hlt_string_size len, int8_t* buffer, __hlt_string_size* bpos, const __hlt_string** dst, __hlt_exception* excpt)
{
    while ( len-- ) {
        _add_char(*data++, buffer, bpos, dst, excpt);
        if ( *excpt )
            return;
    }
}

static void _add_asciiz(const char* asciiz, int8_t* buffer, __hlt_string_size* bpos, const __hlt_string** dst, __hlt_exception* excpt)
{
    while ( *asciiz ) {
        _add_char(*asciiz++, buffer, bpos, dst, excpt);
        if ( *excpt )
            return;
    }
}


static void _do_fmt(const __hlt_string* fmt, const __hlt_type_info* type, void* (*tuple[]), int *type_param, __hlt_string_size* i, int8_t* buffer, __hlt_string_size* bpos, const __hlt_string** dst, __hlt_exception* excpt) {

    const int8_t* p = fmt->bytes;
    
    __hlt_type_info** types = (__hlt_type_info**) &type->type_params;
    __hlt_type_info* fmt_type = types[*type_param];
    void *fmt_arg = (*tuple)[(*type_param)++];
    
    switch ( p[(*i)++] ) {
      case 's': 
        if ( fmt_type->libhilti_fmt ) {
            const __hlt_string* str = (*fmt_type->libhilti_fmt)(fmt_type, fmt_arg, 0, excpt);
            if ( *excpt )
                return;
            
            _add_chars(str->bytes, str->len, buffer, bpos, dst, excpt);
        }
        
        else 
            _add_asciiz(fmt_type->tag, buffer, bpos, dst, excpt);
        
        break;
        
      default:
        // Unknown format character.
        *excpt = __hlt_exception_wrong_arguments;
    }
    
}

const __hlt_string* __hlt_string_sprintf(const __hlt_string* fmt, const __hlt_type_info* type, void* (*tuple[]), __hlt_exception* excpt)
{
    int8_t buffer[BufferSize];
    const __hlt_string* result;
    const int8_t* p = fmt->bytes;
    const __hlt_string* dst = 0;
    int bpos = 0;
    int type_param = 0;
    __hlt_string_size i = 0;

    while ( i < fmt->len ) {
        
        if ( p[i] == '%' ) {
            if ( ++i == fmt->len ) {
                *excpt = __hlt_exception_wrong_arguments;
                return 0;
            }
            
            // Control character.
            if ( p[i] != '%' ) {
                _do_fmt(fmt, type, tuple, &type_param, &i, buffer, &bpos, &dst, excpt);
                if ( *excpt )
                    return 0;
                
                continue;
            }
            
            // Fall-through with quoted '%'.
        }
        
        _add_char(p[i++], buffer, &bpos, &dst, excpt);
        if ( *excpt )
            return 0;
    }
    
    result = __hlt_string_from_data(buffer, bpos, excpt);
    if ( *excpt )
        return 0;
    
    if ( dst )
        result = __hlt_string_concat(dst, result, excpt);
    
    return result;
}
        
