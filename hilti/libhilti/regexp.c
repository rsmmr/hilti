// $Id$

#include "tre/lib/regex.h"

#include "hilti_intern.h"

struct __hlt_regexp {
    __hlt_string pattern;
    regex_t re;
};

static __hlt_string_constant ASCII = { 5, "ascii" };
static __hlt_string_constant EMPTY = { 12, "<no-pattern>" };

__hlt_regexp* __hlt_regexp_new(__hlt_exception* excpt)
{
    __hlt_regexp* re = __hlt_gc_malloc_non_atomic(sizeof(__hlt_regexp));
    re->pattern = 0;
    return re;
}

void __hlt_regexp_compile(__hlt_regexp* re, const __hlt_string pattern, __hlt_exception* excpt)
{
    if ( re->pattern ) {
        regfree(&re->re);
        re->pattern = 0;
    }
    
    // FIXME: For now, the pattern must contain only ASCII characters.
    __hlt_bytes* p = __hlt_string_encode(pattern, &ASCII, excpt);
    if ( *excpt )
        return;
                 
    __hlt_bytes_size plen = __hlt_bytes_len(p, excpt);
    const int8_t* praw = __hlt_bytes_to_raw(p, excpt);
    
    if ( regncomp(&re->re, (const char*)praw, plen, REG_EXTENDED) != 0 ) {
        *excpt = __hlt_exception_pattern_error;
        return;
    }

    if ( tre_have_backrefs(&re->re) ) {
        // FIXME: We don't support back-references for now as we haven't
        // implemented the rewind/compare functions.
        *excpt = __hlt_exception_pattern_error;
        regfree(&re->re);
        return;
    }
    
    re->pattern = pattern;
}

__hlt_string __hlt_regexp_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    const __hlt_regexp* re = *((const __hlt_regexp**)obj);
    return re->pattern ? re->pattern : &EMPTY;
}

// String versions.

int8_t __hlt_regexp_string_find(__hlt_regexp* re, const __hlt_string s, __hlt_exception* excpt)
{
    return 0;
}

__hlt_regexp_range __hlt_regexp_string_span(__hlt_regexp* re, const __hlt_string s, __hlt_exception* excpt)
{
    __hlt_regexp_range span;
    return span;
}

__hlt_vector *__hlt_regexp_string_groups(__hlt_regexp* re, const __hlt_string s, __hlt_exception* excpt)
{
    return 0;
}

// Bytes versions.

struct match_state { 
    __hlt_bytes_pos begin;
    __hlt_bytes_pos end;
    __hlt_bytes_pos cur;
};

static int _get_next_char(tre_char_t *c, unsigned int *pos_add, void *context)
{
    __hlt_exception excpt = 0;
    struct match_state* m = context;
    
    int8_t next = __hlt_bytes_extract_one(&m->cur, m->end, &excpt);
    if ( excpt ) {
        *c = 0;
        return 1;
    }
     
    *c = next;
    *pos_add = 1;
    return 0;
}

static int8_t _match(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, size_t nmatch, regmatch_t *pmatch, __hlt_exception* excpt)
{
    if ( ! re->pattern ) {
        *excpt = __hlt_exception_pattern_error;
        return 0;
    }
    
    struct match_state m;
    m.begin = m.cur = begin;
    m.end = end;
    
    tre_str_source src;
    src.get_next_char = _get_next_char;
    src.rewind = 0;
    src.compare = 0;
    src.context = &m;
    
    int rc = reguexec(&re->re, &src, nmatch, pmatch, 0);

    if ( rc == REG_ESPACE ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }
    
    return rc ? 0 : 1;
}

int8_t __hlt_regexp_bytes_find(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt)
{
    return _match(re, begin, end, 0, 0, excpt);
}
    
__hlt_regexp_range __hlt_regexp_bytes_span(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt)
{
    regmatch_t pmatch[1];
    __hlt_regexp_range span;

    int8_t rc = _match(re, begin, end, 1, pmatch, excpt);

    if ( rc ) {
        span.begin = __hlt_bytes_pos_incr_by(begin, pmatch[0].rm_so, excpt);
        span.end = __hlt_bytes_pos_incr_by(begin, pmatch[0].rm_eo, excpt);
    }
    
    else
        span.begin = span.end = __hlt_bytes_end(excpt);
    
    return span;
}

extern const __hlt_type_info __hlt_type_info_tuple_iterator_bytes_iterator_bytes;

__hlt_vector *__hlt_regexp_bytes_groups(__hlt_regexp* re, const __hlt_bytes_pos begin, const __hlt_bytes_pos end, __hlt_exception* excpt)
{
    static const int MAX_GROUPS = 10;
    regmatch_t pmatch[MAX_GROUPS];
    
    __hlt_regexp_range def_span;
    def_span.begin = def_span.end = __hlt_bytes_end(excpt);

    __hlt_vector* vec = __hlt_vector_new(&__hlt_type_info_tuple_iterator_bytes_iterator_bytes, &def_span, excpt);

    int8_t rc = _match(re, begin, end, 10, pmatch, excpt);

    if ( ! rc )
        return vec;
    
    for ( int i = 0; i < MAX_GROUPS; i++ ) {
        
        if ( pmatch[i].rm_so == -1 )
            // Not set.
            continue;
        
        __hlt_regexp_range span;
        span.begin = __hlt_bytes_pos_incr_by(begin, pmatch[i].rm_so, excpt);
        span.end = __hlt_bytes_pos_incr_by(begin, pmatch[i].rm_eo, excpt);
        
        __hlt_vector_set(vec, i, &__hlt_type_info_tuple_iterator_bytes_iterator_bytes, &span, excpt);
    }
    
    return vec;
}
    
