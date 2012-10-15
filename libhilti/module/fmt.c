//
// Printf-style fmt() function.
//
// Todo: We currently understand only the %s format and no formatting
// options.

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "module.h"
#include "string_.h"
#include "rtti.h"
#include "exceptions.h"

static const int BufferSize = 128;

static void _add_char(int8_t c, int8_t* buffer, hlt_string_size* bpos, hlt_string* dst, hlt_exception** excpt, hlt_execution_context* ctx)
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
        hlt_string new_dst = hlt_string_from_data(buffer, *bpos, excpt, ctx);
        if ( *excpt )
            return;

        new_dst = hlt_string_concat(*dst, new_dst, excpt, ctx);
        buffer[0] = c;
        *bpos = 1;
        GC_DTOR(*dst, hlt_string);
        *dst = new_dst;
    }
}

static void _add_chars(const int8_t* data, hlt_string_size len, int8_t* buffer, hlt_string_size* bpos, hlt_string* dst, hlt_exception** excpt, hlt_execution_context* ctx)
{
    while ( len-- ) {
        _add_char(*data++, buffer, bpos, dst, excpt, ctx);
        if ( *excpt )
            return;
    }
}

static void _add_asciiz(const char* asciiz, int8_t* buffer, hlt_string_size* bpos, hlt_string* dst, hlt_exception** excpt, hlt_execution_context* ctx)
{
    while ( *asciiz ) {
        _add_char(*asciiz++, buffer, bpos, dst, excpt, ctx);
        if ( *excpt )
            return;
    }
}

static void _do_fmt(hlt_string fmt, const hlt_type_info* type, const void* tuple, int *type_param, hlt_string_size* i, int8_t* buffer, hlt_string_size* bpos, hlt_string* dst, hlt_exception** excpt, hlt_execution_context* ctx)
{
    static const int tmp_size = 32;
    char tmp[tmp_size];

    const int8_t* p = fmt->bytes;

    int16_t* offsets = (int16_t *)type->aux;

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    hlt_type_info* fmt_type = types[*type_param];
    const void *fmt_arg = *type_param < type->num_params ? tuple + offsets[(*type_param)++] : 0;

    switch ( p[(*i)++] ) {

      case 'd':
        if ( fmt_type->to_int64 ) {
            int64_t i = (*fmt_type->to_int64)(fmt_type, fmt_arg, HLT_CONVERT_NONE, excpt, ctx);
            if ( *excpt )
                return;

            snprintf(tmp, tmp_size, "%" PRId64, i);
            _add_asciiz(tmp, buffer, bpos, dst, excpt, ctx);
        }
        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }
        break;

      case 'u':
        if ( fmt_type->to_int64 ) {
            uint64_t i = (uint64_t)(*fmt_type->to_int64)(fmt_type, fmt_arg, HLT_CONVERT_UNSIGNED, excpt, ctx);
            if ( *excpt )
                return;

            snprintf(tmp, tmp_size, "%" PRIu64, i);
            _add_asciiz(tmp, buffer, bpos, dst, excpt, ctx);
        }
        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        break;

      case 'x':
        if ( fmt_type->to_int64 ) {
            uint64_t i = (uint64_t)(*fmt_type->to_int64)(fmt_type, fmt_arg, HLT_CONVERT_UNSIGNED, excpt, ctx);
            if ( *excpt )
                return;

            snprintf(tmp, tmp_size, "%" PRIx64, i);
            _add_asciiz(tmp, buffer, bpos, dst, excpt, ctx);
        }
        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        break;

      case 'f':
        if ( fmt_type->to_double ) {
            double d = (*fmt_type->to_double)(fmt_type, fmt_arg, HLT_CONVERT_NONE, excpt, ctx);
            if ( *excpt )
                return;

            snprintf(tmp, tmp_size, "%f", d);
            _add_asciiz(tmp, buffer, bpos, dst, excpt, ctx);
        }
        else {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        break;

      case 'p':
        if ( fmt_arg && * (void **)fmt_arg ) {
            snprintf(tmp, tmp_size, "%p", * (void **)fmt_arg);
            _add_asciiz(tmp, buffer, bpos, dst, excpt, ctx);
        }
        else
            // Printing null pointers yield different output with differenet
            // libs.  Canonicalize the output.
            _add_asciiz("0x0", buffer, bpos, dst, excpt, ctx);

        break;

      case 's':
        if ( fmt_type->to_string ) {
            hlt_string str = (*fmt_type->to_string)(fmt_type, fmt_arg, HLT_CONVERT_NONE, excpt, ctx);
            if ( *excpt )
                return;

            if ( str ) {
                _add_chars(str->bytes, str->len, buffer, bpos, dst, excpt, ctx);
                GC_DTOR(str, hlt_string);
            }
        }

        else
            _add_asciiz(fmt_type->tag, buffer, bpos, dst, excpt, ctx);

        break;

      default:
        // Unknown format character.
        hlt_set_exception(excpt, &hlt_exception_wrong_arguments, 0);
    }

}

hlt_string hilti_fmt(hlt_string fmt, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TUPLE);

    int8_t buffer[BufferSize];
    hlt_string result;
    const int8_t* p = fmt->bytes;
    hlt_string dst = 0;
    int64_t bpos = 0;
    int type_param = 0;
    hlt_string_size i = 0;

    while ( i < fmt->len ) {

        if ( p[i] == '%' ) {
            if ( ++i == fmt->len ) {
                hlt_set_exception(excpt, &hlt_exception_wrong_arguments, 0);
                return 0;
            }

            // Control character.
            if ( p[i] != '%' ) {
                _do_fmt(fmt, type, tuple, &type_param, &i, buffer, &bpos, &dst, excpt, ctx);
                if ( *excpt )
                    return 0;

                continue;
            }

            // Fall-through with quoted '%'.
        }

        _add_char(p[i++], buffer, &bpos, &dst, excpt, ctx);
        if ( *excpt )
            return 0;
    }

    result = hlt_string_from_data(buffer, bpos, excpt, ctx);
    if ( *excpt )
        return 0;

    if ( dst ) {
        hlt_string nresult = hlt_string_concat(dst, result, excpt, ctx);
        GC_DTOR(result, hlt_string);
        result = nresult;
    }

    return result;
}


