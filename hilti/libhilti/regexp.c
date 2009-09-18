// $Id$
//
// TODO: Need to register cleanup with GC.

#include <string.h>
#include <jrx.h>

#include "hilti.h"

struct hlt_regexp {
    int32_t num; // Number of patterns in set.
    regex_t regexp;
    hlt_string* patterns;
};

static hlt_string_constant ASCII = { 5, "ascii" };
static hlt_string_constant EMPTY = { 12, "<no-pattern>" };
static hlt_string_constant PIPE = { 4, " | " };

hlt_regexp* hlt_regexp_new(hlt_exception* excpt)
{
    hlt_regexp* re = hlt_gc_malloc_non_atomic(sizeof(hlt_regexp));
    re->num = 0;
    re->patterns = 0;
    return re;
}

static void _compile_one(hlt_regexp* re, hlt_string pattern, int idx, hlt_exception* excpt)
{
#if 0    
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_bytes* p = hlt_string_encode(pattern, &ASCII, excpt);
    if ( *excpt )
        return;
    
    hlt_bytes_size plen = hlt_bytes_len(p, excpt);
    const int8_t* praw = hlt_bytes_to_raw(p, excpt);
    
    if ( regset_add(&re->regexp, (const char*)praw, plen) != 0 ) {
        *excpt = hlt_exception_pattern_error;
        return;
    }
    
    re->patterns[idx] = pattern;
#endif    
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception* excpt)
{
    if ( re->num != 0 ) {
        *excpt = hlt_exception_value_error;
        return;
    }

#if 0    
    re->num = 1;
    re->patterns = hlt_gc_malloc_non_atomic(sizeof(hlt_string));
    regset_init(&re->regexp, 0);
    _compile_one(re, pattern, 0, excpt);
    regset_finalize(&re->regexp);
#endif    
}

void hlt_regexp_compile_set(hlt_regexp* re, hlt_list* patterns, hlt_exception* excpt)
{
#if 0    
    if ( re->num != 0 ) {
        *excpt = hlt_exception_value_error;
        return;
    }
    
    re->num = hlt_list_size(patterns, excpt);
    re->patterns = hlt_gc_malloc_non_atomic(re->num * sizeof(hlt_string));
    regset_init(&re->regexp, 0);

    hlt_list_iter i = hlt_list_begin(patterns, excpt);
    hlt_list_iter end = hlt_list_end(patterns, excpt);
    int idx = 0;
    
    while ( ! hlt_list_iter_eq(i, end, excpt) ) {
        hlt_string* pattern = hlt_list_iter_deref(i, excpt);
        _compile_one(re, *pattern, idx, excpt);
        
        if ( *excpt )
            return;
        
        i = hlt_list_iter_incr(i, excpt);
        idx++;
    }
    
    regset_finalize(&re->regexp);
#endif    
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt)
{
    const hlt_regexp* re = *((const hlt_regexp**)obj);
    
    if ( ! re->num )
        return &EMPTY;
    
    if ( re->num == 1 )
        return re->patterns[0];
    
    hlt_string s = hlt_string_from_asciiz("", excpt);
    
    for ( int32_t idx = 0; idx < re->num; idx++ ) {
        if ( idx > 0 )
            s = hlt_string_concat(s, &PIPE, excpt);
        s = hlt_string_concat(s, re->patterns[idx], excpt);
    }
    
    return s;
}

// String versions (not implemented yet).

int32_t hlt_regexp_string_find(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    *excpt = hlt_exception_not_implemented;
    return 0;
}

hlt_regexp_span_result hlt_regexp_string_span(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    *excpt = hlt_exception_not_implemented;
    hlt_regexp_span_result span;
    return span;
}

hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    *excpt = hlt_exception_not_implemented;
    return 0;
}

// Bytes versions.
struct match_state { 
    hlt_bytes_pos begin;
    hlt_bytes_pos end;
    hlt_bytes_pos cur;
};

static jrx_accept_id _search_pattern(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, 
                                     jrx_offset* so, jrx_offset* eo, hlt_exception* excpt)
{
    return 0;
}

int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
    if ( ! re->num ) {
        *excpt = hlt_exception_pattern_error;
        return 0;
    }
    
    return _search_pattern(re, begin, end, 0, 0, excpt);
}
    
hlt_regexp_span_result hlt_regexp_bytes_span(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
    hlt_regexp_span_result result;
    
    if ( ! re->num ) {
        *excpt = hlt_exception_pattern_error;
        return result;
    }

    jrx_offset so = 0;
    jrx_offset eo = 0;
    int rc = _search_pattern(re, begin, end, &so, &eo, excpt);
    
    if ( rc > 0 ) {
        result.span.begin = hlt_bytes_pos_incr_by(begin, so, excpt);
        result.span.end = hlt_bytes_pos_incr_by(result.span.begin, eo - so, excpt);
    }
    
    else
        result.span.begin = result.span.end = hlt_bytes_end(excpt);
    
    return result;
}

extern const hlt_type_info hlt_type_info_tuple_iterator_bytes_iterator_bytes;

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
#if 0    
    if ( re->num != 1 ) {
        // No support for sets.
        *excpt = hlt_exception_pattern_error;
        return 0;
    }
    
    static const int MAX_GROUPS = 10;
    regmatch_t pmatch[MAX_GROUPS];
    
    hlt_regexp_range def_span;
    def_span.begin = def_span.end = hlt_bytes_end(excpt);

    hlt_vector* vec = hlt_vector_new(&hlt_type_info_tuple_iterator_bytes_iterator_bytes, &def_span, excpt);

    int8_t rc = _match(re, begin, end, 10, pmatch, excpt);

    if ( ! rc )
        return vec;
    
    for ( int i = 0; i < MAX_GROUPS; i++ ) {
        
        if ( pmatch[i].rm_so == -1 )
            // Not set.
            continue;
        
        hlt_regexp_range span;
        span.begin = hlt_bytes_pos_incr_by(begin, pmatch[i].rm_so, excpt);
        span.end = hlt_bytes_pos_incr_by(begin, pmatch[i].rm_eo, excpt);
        
        hlt_vector_set(vec, i, &hlt_type_info_tuple_iterator_bytes_iterator_bytes, &span, excpt);
    }
    
    return vec;
#endif
}
    
