/* $Id$
 * 
 * Printf-style fmt() function.
 * 
 * Todo: We currently understand only the %s format and not formatting
 * options. 
 * 
 */

#include <stdio.h> 
 
#include "hilti_intern.h"

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

static void _do_fmt(const __hlt_string* fmt, const __hlt_type_info* type, const char* tuple, int *type_param, __hlt_string_size* i, int8_t* buffer, __hlt_string_size* bpos, const __hlt_string** dst, __hlt_exception* excpt) 
{
    static const int tmp_size = 32;
    char tmp[tmp_size];
    
    const int8_t* p = fmt->bytes;
    
    int16_t* offsets = (int16_t *)type->aux;

    __hlt_type_info** types = (__hlt_type_info**) &type->type_params;
    __hlt_type_info* fmt_type = types[*type_param];
    const void *fmt_arg = tuple + offsets[(*type_param)++];
    
    switch ( p[(*i)++] ) {
        
      case 'd': 
        if ( fmt_type->to_int64 ) {
            int64_t i = (*fmt_type->to_int64)(fmt_type, fmt_arg, excpt);
            if ( *excpt )
                return;
            
            snprintf(tmp, tmp_size, "%lld", i);
            _add_asciiz(tmp, buffer, bpos, dst, excpt);
        }
        else {
            *excpt = __hlt_exception_value_error;
            return;
        }
        
        break;
    
      case 'f': 
        if ( fmt_type->to_double ) {
            double d = (*fmt_type->to_double)(fmt_type, fmt_arg, excpt);
            if ( *excpt )
                return;
            
            snprintf(tmp, tmp_size, "%f", d);
            _add_asciiz(tmp, buffer, bpos, dst, excpt);
        }
        else {
            *excpt = __hlt_exception_value_error;
            return;
        }
        
        break;
    
      case 'p': 
        if ( * (void **)fmt_arg ) {
            snprintf(tmp, tmp_size, "%p", * (void **)fmt_arg);
            _add_asciiz(tmp, buffer, bpos, dst, excpt);
        }
        else 
            // Printing null pointers yield different output with differenet
            // libs.  Canonicalize the output.
            _add_asciiz("0x0", buffer, bpos, dst, excpt);
        
        break;
        
      case 's': 
        if ( fmt_type->to_string ) {
            const __hlt_string* str = (*fmt_type->to_string)(fmt_type, fmt_arg, 0, excpt);
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

const __hlt_string* hilti_fmt(const __hlt_string* fmt, const __hlt_type_info* type, const char* tuple, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_TUPLE);

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
        

