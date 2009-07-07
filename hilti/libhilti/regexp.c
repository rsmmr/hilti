// $Id$

#include "tre/lib/regex.h"

#include "hilti.h"

struct hlt_regexp {
    hlt_string pattern;
    regex_t re;
};

static hlt_string_constant ASCII = { 5, "ascii" };
static hlt_string_constant EMPTY = { 12, "<no-pattern>" };

hlt_regexp* hlt_regexp_new(hlt_exception* excpt)
{
    hlt_regexp* re = hlt_gc_malloc_non_atomic(sizeof(hlt_regexp));
    re->pattern = 0;
    return re;
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception* excpt)
{
    if ( re->pattern ) {
        regfree(&re->re);
        re->pattern = 0;
    }
    
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_bytes* p = hlt_string_encode(pattern, &ASCII, excpt);
    if ( *excpt )
        return;
                 
    hlt_bytes_size plen = hlt_bytes_len(p, excpt);
    const int8_t* praw = hlt_bytes_to_raw(p, excpt);
    
    if ( regncomp(&re->re, (const char*)praw, plen, REG_EXTENDED) != 0 ) {
        *excpt = hlt_exception_pattern_error;
        return;
    }

    if ( tre_have_backrefs(&re->re) ) {
        // FIXME: We don't support back-references for now as we haven't
        // implemented the rewind/compare functions.
        *excpt = hlt_exception_pattern_error;
        regfree(&re->re);
        return;
    }
    
    re->pattern = pattern;
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt)
{
    const hlt_regexp* re = *((const hlt_regexp**)obj);
    return re->pattern ? re->pattern : &EMPTY;
}

// String versions.

int8_t hlt_regexp_string_find(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    return 0;
}

hlt_regexp_range hlt_regexp_string_span(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    hlt_regexp_range span;
    return span;
}

hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, const hlt_string s, hlt_exception* excpt)
{
    return 0;
}

// Bytes versions.

struct match_state { 
    hlt_bytes_pos begin;
    hlt_bytes_pos end;
    hlt_bytes_pos cur;
};

static int _get_next_char(tre_char_t *c, unsigned int *pos_add, void *context)
{
    hlt_exception excpt = 0;
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

static int8_t _match(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, size_t nmatch, regmatch_t *pmatch, hlt_exception* excpt)
{
    if ( ! re->pattern ) {
        *excpt = hlt_exception_pattern_error;
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
        *excpt = hlt_exception_out_of_memory;
        return 0;
    }
    
    return rc ? 0 : 1;
}

int8_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
    return _match(re, begin, end, 0, 0, excpt);
}
    
hlt_regexp_range hlt_regexp_bytes_span(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
    regmatch_t pmatch[1];
    hlt_regexp_range span;

    int8_t rc = _match(re, begin, end, 1, pmatch, excpt);

    if ( rc ) {
        span.begin = hlt_bytes_pos_incr_by(begin, pmatch[0].rm_so, excpt);
        span.end = hlt_bytes_pos_incr_by(begin, pmatch[0].rm_eo, excpt);
    }
    
    else
        span.begin = span.end = hlt_bytes_end(excpt);
    
    return span;
}

extern const hlt_type_info hlt_type_info_tuple_iterator_bytes_iterator_bytes;

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception* excpt)
{
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
}
    
