///
/// Support functions HILTI's string data type.
///
/// Note that we treat a null pointer as a legal representation of the empty
/// string. That simplifies its handling as a constant.
///

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include <assert.h>
#include <string.h>

#include "string_.h"
#include "types.h"
#include "exceptions.h"
#include "bytes.h"
#include "rtti.h"
#include "memory_.h"
#include "utf8proc.h"
#include "hutil.h"
#include "autogen/hilti-hlt.h"
#include "3rdparty/convertutf/ConvertUTF.h"

void hlt_string_dtor(hlt_type_info* ti, hlt_string* s)
{
    // Nothing to do.
}

hlt_string hlt_string_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = *((hlt_string*)obj);
    GC_CCTOR(s, hlt_string);
    return s ? s : 0;
}

hlt_hash hlt_string_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = *((hlt_string*)obj);
    return s ? hlt_hash_bytes(s->bytes, s->len) : 0;
}

int8_t hlt_string_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s1 = *((hlt_string*)obj1);
    hlt_string s2 = *((hlt_string*)obj2);

    if ( ! (s1 && s2) )
        return s1 == s2;

    if ( s1->len != s2->len )
        return 0;

    return memcmp(s1->bytes, s2->bytes, s1->len) == 0;
}

hlt_string_size hlt_string_len(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int32_t dummy;
    const int8_t* p;
    const int8_t* e;

    if ( ! s )
        return 0;

    p = s->bytes;
    e = p + s->len;

    hlt_string_size len = 0;

    if ( ! s )
        return 0;

    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        ++len;
        p += n;
    }

    if ( p != e ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return 0;
    }

    return len;
}

#include <stdio.h>

hlt_string hlt_string_concat(hlt_string s1, hlt_string s2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string_size len1;
    hlt_string_size len2;;

    len1 = s1 ? s1->len : 0;
    len2 = s2 ? s2->len : 0;

    if ( ! len1 ) {
        GC_CCTOR(s2, hlt_string);
        return s2;
    }

    if ( ! len2 ) {
        GC_CCTOR(s1, hlt_string);
        return s1;
    }

    hlt_string dst = GC_NEW_CUSTOM_SIZE(hlt_string, sizeof(struct __hlt_string) + len1 + len2);

    if ( ! dst ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    dst->len = len1 + len2;
    memcpy(dst->bytes, s1->bytes, len1);
    memcpy(dst->bytes + len1, s2->bytes, len2);

    return dst;
}

hlt_string hlt_string_concat_and_unref(hlt_string s1, hlt_string s2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = hlt_string_concat(s1, s2, excpt, ctx);

    GC_DTOR(s1, hlt_string);
    GC_DTOR(s2, hlt_string);

    return s;
}

hlt_string hlt_string_substr(hlt_string s, hlt_string_size pos, hlt_string_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int32_t dummy;
    const int8_t* f;
    const int8_t* p;
    const int8_t* e;

    if ( ! s )
        return 0;

    if ( ! (len && s->len) )
        return 0;

    p = s->bytes;
    e = p + s->len;

    while ( p < e && pos ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        --pos;
        p += n;
    }

    f = p;

    while ( p < e && len ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        --len;
        p += n;
    }

    if ( p == f )
        return 0;

    return hlt_string_from_data(f, p - f, excpt, ctx);
}

hlt_string_size hlt_string_find(hlt_string s, hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int32_t dummy;
    const int8_t* e;
    const int8_t* p;

    int slen = s ? s->len : 0;
    int patlen = pattern ? pattern->len : 0;

    if ( slen == 0 && patlen == 0 )
        return 0;

    if ( slen == 0 )
        return -1;

    if ( patlen == 0 )
        return 0;

    p = s->bytes;
    e = p + s->len;
    int pos = 0;

    while ( p < e ) {
        if ( memcmp(p, pattern->bytes, pattern->len) == 0 )
            return pos;

        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &dummy);
        p += n;
        ++pos;
    }

    return -1;
}

int8_t hlt_string_cmp(hlt_string s1, hlt_string s2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const int8_t* p1;
    const int8_t* p2;

    size_t len1 = s1 ? s1->len : 0;
    size_t len2 = s2 ? s2->len : 0;

    if ( len1 == 0 && len2 == 0 )
        return 0;

    if ( len1 == 0 )
        return -1;

    if ( len2 == 0 )
        return 1;

    p1 = s1->bytes;
    p2 = s2->bytes;

    hlt_string_size i = 0;

    while ( true ) {
        ssize_t n1, n2;
        int32_t cp1, cp2;

        n1 = utf8proc_iterate((uint8_t *)p1, s1->len - i, &cp1);
        if ( n1 < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return 0;
        }

        n2 = utf8proc_iterate((uint8_t *)p2, s2->len - i, &cp2);
        if ( n2 < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
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

hlt_string hlt_string_empty(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return 0; // Null is a valid representation of the empty string.
}

hlt_string hlt_string_copy(hlt_string src, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_string_from_data(src->bytes, src->len, excpt, ctx);
}

hlt_string hlt_string_from_asciiz(const char* asciiz, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string_size len = strlen(asciiz);
    hlt_string dst = GC_NEW_CUSTOM_SIZE(hlt_string, sizeof(struct __hlt_string) + len);
    dst->len = len;
    memcpy(&dst->bytes, asciiz, len);
    return dst;
}

hlt_string hlt_string_from_data(const int8_t* data, hlt_string_size len, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string dst = GC_NEW_CUSTOM_SIZE(hlt_string, sizeof(struct __hlt_string) + len);
    dst->len = len;
    memcpy(dst->bytes, data, len);
    return dst;
}

hlt_string hlt_object_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(seen);

    if ( type->gc && type->to_string ) {
        void *ptr = *(void **)obj;
        if ( __hlt_pointer_stack_lookup(seen, ptr) )
            return hlt_string_from_asciiz("[...recursion...]", excpt, ctx);

        __hlt_pointer_stack_push_back(seen, ptr);
    }

    hlt_string s = 0;

    if ( type->to_string )
        s = (*type->to_string)(type, obj, options, seen, excpt, ctx);
    else
        s = hlt_string_from_asciiz(type->tag, excpt, ctx);

    if ( type->gc && type->to_string )
        __hlt_pointer_stack_pop_back(seen);

    return s;
}

enum Charset { ERROR, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE, ASCII };

static enum Charset get_charset(hlt_enum charset, hlt_exception** excpt, hlt_execution_context* ctx)
{
    enum Charset cs = ERROR;

    if ( hlt_enum_equal(charset, Hilti_Charset_UTF8, excpt, ctx) )
        cs = UTF8;

    else if ( hlt_enum_equal(charset, Hilti_Charset_UTF16LE, excpt, ctx) )
        cs = UTF16LE;

    else if ( hlt_enum_equal(charset, Hilti_Charset_UTF16BE, excpt, ctx) )
        cs = UTF16BE;

    else if ( hlt_enum_equal(charset, Hilti_Charset_UTF32LE, excpt, ctx) )
        cs = UTF32LE;

    else if ( hlt_enum_equal(charset, Hilti_Charset_UTF32BE, excpt, ctx) )
        cs = UTF32BE;

    else if ( hlt_enum_equal(charset, Hilti_Charset_ASCII, excpt, ctx) )
        cs = ASCII;

    else
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);

    return cs;
}

hlt_bytes* hlt_string_encode(hlt_string s, hlt_enum charset, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const int8_t* p;
    const int8_t* e;

    enum Charset ch = get_charset(charset, excpt, ctx);

    if ( ch == ERROR )
        return 0;

    hlt_bytes* dst = hlt_bytes_new(excpt, ctx);

    if ( ! s )
        return dst;

    p = s->bytes;
    e = p + s->len;

    if ( ch == UTF8 ) {
        // Caller wants UTF-8 so we just need append it literally.
        int8_t* data = hlt_malloc(s->len);
        memcpy(data, s->bytes, s->len);
        hlt_bytes_append_raw(dst, data, s->len, excpt, ctx);
        return dst;
    } else if ( ch == ASCII ) {
        // fallthrough
    } else {
        hlt_string err = hlt_string_from_asciiz("charset not supported for encode", excpt, ctx);
        hlt_set_exception(excpt, &hlt_exception_conversion_error, err);
        return 0;
    }

    static const int BUFFER_SIZE = 128;

    int8_t* data = 0;
    int i = 0;

    while ( p < e ) {

        int32_t uc;
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &uc);

        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            GC_DTOR(dst, hlt_bytes);
            return 0;
        }

        if ( ! data ) {
            data = hlt_malloc(BUFFER_SIZE);
            i = 0;
        }

        switch ( ch ) {
          case ASCII: {
              // Replace non-ASCII characters with '?'.
              int8_t c = uc < 128 ? (int8_t)uc : '?';
              data[i++] = c;
              break;
          }

          default:
            assert(false);
        }

        p += n;

        if ( data && (i == BUFFER_SIZE || p >= e) ) {
            hlt_bytes_append_raw(dst, data, i, excpt, ctx);
            data = 0;
        }
    }

    if ( data )
        hlt_free(data);

    return dst;
}

hlt_string hlt_string_decode(hlt_bytes* b, hlt_enum charset, hlt_exception** excpt, hlt_execution_context* ctx)
{
    enum Charset ch = get_charset(charset, excpt, ctx);
    if ( ch == ERROR )
        return 0;

    if ( hlt_bytes_empty(b, excpt, ctx) )
        return 0;

    if ( ch == UTF8 ) {
        // Data is already in UTF-8, just need to copy it into a string.
        hlt_iterator_bytes begin = hlt_bytes_begin(b, excpt, ctx);
        hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);
        int8_t* raw = hlt_bytes_sub_raw(begin, end, excpt, ctx);
        const hlt_string dst = hlt_string_from_data(raw, hlt_bytes_len(b, excpt, ctx), excpt, ctx);
        hlt_free(raw);
        GC_DTOR(begin, hlt_iterator_bytes);
        GC_DTOR(end, hlt_iterator_bytes);
        return dst;
    } else if ( ch == UTF16LE || ch == UTF16BE || ch == UTF32LE || ch == UTF32BE ) {
        uint8_t do_flip = 0;

#if __BIG_ENDIAN__
        if ( ch == UTF16LE || ch == UTF32LE )
            do_flip=1;
#else
        if ( ch == UTF16BE || ch == UTF32BE )
            do_flip=1;
#endif

        hlt_iterator_bytes begin = hlt_bytes_begin(b, excpt, ctx);
        hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);
        int8_t* raw = hlt_bytes_sub_raw(begin, end, excpt, ctx);

        // Determining UTF-8 sizes is quite difficult due to variable-length characters. If our start-
        // encoding is UTF-16, the worst-case is that the UTF-8 string has double the length of the
        // UTF-16 string (meaning each character is represented in the maximum UTF-8 length of 4 bytes).
        // For UTF-32, the maximum size is the same size as the string;

        hlt_bytes_size source_len = hlt_bytes_len(b, excpt, ctx);
        hlt_bytes_size buffer_len = 2*source_len+1;
        if ( ch == UTF32BE || ch == UTF32LE )
                buffer_len = source_len+1;

        int8_t buffer[buffer_len];

        const int8_t* source_start_ptr = raw;
        const int8_t** source_start = &source_start_ptr;
        const int8_t* source_end = &raw[source_len]; // source_end has to point to a char after the last input character

        int8_t* target_start_ptr = buffer;
        int8_t** target_start = &target_start_ptr;
        int8_t* target_end = target_start_ptr + buffer_len;

        hilti_ConversionResult res;
        if ( ch == UTF16LE || ch == UTF16BE )
            res = hilti_ConvertUTF16toUTF8((const UTF16_t**) source_start, (const UTF16_t*) source_end, (UTF8_t**) target_start, (UTF8_t*) target_end, lenientConversion, do_flip);
        else
            res = hilti_ConvertUTF32toUTF8((const UTF32_t**) source_start, (const UTF32_t*) source_end, (UTF8_t**) target_start, (UTF8_t*) target_end, lenientConversion, do_flip);

        if ( res != conversionOK ) {
            // something went wrong...
            hlt_string err;

            switch ( res ) {
                case sourceExhausted:
                    err = hlt_string_from_asciiz("source string ends with partial character", excpt, ctx);
                    break;
                case sourceIllegal:
                    err = hlt_string_from_asciiz("malformed source string", excpt, ctx);
                    break;
                default:
                    err = hlt_string_from_asciiz("internal string conversion error", excpt, ctx);
            }

            hlt_set_exception(excpt, &hlt_exception_conversion_error, err);
        }

        hlt_bytes_size final_length = target_start_ptr - buffer; // target_start points to last processed character
        const hlt_string dst = hlt_string_from_data(buffer, final_length, excpt, ctx);

        hlt_free(raw);
        GC_DTOR(begin, hlt_iterator_bytes);
        GC_DTOR(end, hlt_iterator_bytes);
        return dst;
    } else if ( ch == ASCII ) {
        // Convert all bytes to 7-bit codepoints.
        hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
        hlt_iterator_bytes end = hlt_bytes_end(b, excpt, ctx);
        hlt_string dst = GC_NEW_CUSTOM_SIZE(hlt_string, sizeof(struct __hlt_string) + hlt_bytes_len(b, excpt, ctx));
        dst->len = len;
        int8_t* p = dst->bytes;

        hlt_iterator_bytes i = hlt_bytes_begin(b, excpt, ctx);
        hlt_iterator_bytes j;
        while ( ! hlt_iterator_bytes_eq(i, end, excpt, ctx) ) {
            char c = hlt_iterator_bytes_deref(i, excpt, ctx);
            *p++ = (c & 0x7f) == c ? c : '?';
            j = hlt_iterator_bytes_incr(i, excpt, ctx);
            GC_DTOR(i, hlt_iterator_bytes);
            i = j;
        }

        GC_DTOR(i, hlt_iterator_bytes);
        GC_DTOR(end, hlt_iterator_bytes);

        return dst;
    }

    assert(false); // cannot be reached.
    return 0;
}

/* FIXME: This function doesn't print non-ASCII Unicode codepoints as we can't
 * convert to the locale encoding yet. We just print them in \u syntax. */
void hlt_string_print_n(FILE* file, hlt_string s, int8_t newline, hlt_string_size n, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! s )
        // Empty string.
        return;

    hlt_string_size len = s->len <= n ? s->len : n;

    int32_t cp;
    const int8_t* p = s->bytes;
    const int8_t* e = p + len;

    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &cp);

        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0);
            return;
        }

        if ( cp < 128 )
            fputc(cp, file);
        else {
            // FIXME: We should bring our own itoa().
            if ( cp < (1 << 16) )
                fprintf(file, "\\u%04x", cp);
            else
                fprintf(file, "\\U%08x", cp);
        }

        p += n;
    }

    if ( s->len > n )
        fprintf(stderr, "...");

    if ( newline )
        fputc('\n', file);

}

void hlt_string_print(FILE* file, hlt_string s, int8_t newline, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! s )
        // Empty string.
        return;

    hlt_string_print_n(file, s, newline, s->len, excpt, ctx);
}

/* FIXME: We don't really do "to native" yet, but just to ASCII ...
 * This is also all very inefficient ...
*/
char* hlt_string_to_native(hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_string_encode(s, Hilti_Charset_ASCII, excpt, ctx);

    // Add a null terminator.
    hlt_bytes_append_raw_copy(b, (int8_t*)"\0", 1, excpt, ctx);

    if ( *excpt )
        return 0;

    const int8_t* raw = hlt_bytes_to_raw(b, excpt, ctx);
    GC_DTOR(b, hlt_bytes);

    if ( *excpt )
        return 0;

    return (char*)raw;
}

hlt_string hlt_string_join(hlt_string sep, hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_type_info* ti = hlt_list_type(l, excpt, ctx);
    hlt_iterator_list i = hlt_list_begin(l, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(l, excpt, ctx);

    int first = 1;
    hlt_string s = 0;
	hlt_string old;

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {

        if ( ! first ) {
            old = s;
            s = hlt_string_concat(s, sep, excpt, ctx);
            GC_DTOR(old, hlt_string);
        }

        void* obj = hlt_iterator_list_deref(i, excpt, ctx);

        __hlt_pointer_stack* seen = __hlt_pointer_stack_new();
        hlt_string so = hlt_object_to_string(ti, obj, 0, seen, excpt, ctx);
        __hlt_pointer_stack_delete(seen);

        s = hlt_string_concat_and_unref(s, so, excpt, ctx);

        GC_DTOR_GENERIC(obj, ti);

        hlt_iterator_list j = i;
        i = hlt_iterator_list_incr(i, excpt, ctx);
        GC_DTOR(j, hlt_iterator_list);

        first = 0;
    }

    return s;
}
