// $Id$
//
// TODO: Need to register cleanup with GC.

#include <string.h>
#include <jrx.h>

#include "hilti.h"

struct hlt_regexp {
    int32_t num; // Number of patterns in set.
    hlt_string* patterns;
    jrx_regex_t regexp;
};

static hlt_string_constant ASCII = { 5, "ascii" };
static hlt_string_constant EMPTY = { 12, "<no-pattern>" };
static hlt_string_constant PIPE = { 4, " | " };

hlt_regexp* hlt_regexp_new(hlt_exception** excpt)
{
    hlt_regexp* re = hlt_gc_malloc_non_atomic(sizeof(hlt_regexp));
    re->num = 0;
    re->patterns = 0;
    return re;
}

static inline int _anchor(int cflags) 
{
    return cflags | ((cflags & REG_NOSUB) ? REG_ANCHOR : 0);
}

static void _compile_one(hlt_regexp* re, hlt_string pattern, int idx, hlt_exception** excpt)
{
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_bytes* p = hlt_string_encode(pattern, &ASCII, excpt);
    if ( *excpt )
        return;

    hlt_bytes_size plen = hlt_bytes_len(p, excpt);
    const int8_t* praw = hlt_bytes_to_raw(p, excpt);
    
    if ( jrx_regset_add(&re->regexp, (const char*)praw, plen) != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return;
    }
    
    re->patterns[idx] = pattern;
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception** excpt)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    re->num = 1;
    re->patterns = hlt_gc_malloc_non_atomic(sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _anchor(REG_EXTENDED));
    _compile_one(re, pattern, 0, excpt);
    jrx_regset_finalize(&re->regexp);
}

void hlt_regexp_compile_set(hlt_regexp* re, hlt_list* patterns, hlt_exception** excpt)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }
    
    re->num = hlt_list_size(patterns, excpt);
    re->patterns = hlt_gc_malloc_non_atomic(re->num * sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _anchor(REG_EXTENDED));

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
    
    jrx_regset_finalize(&re->regexp);
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt)
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

int32_t hlt_regexp_string_find(hlt_regexp* re, const hlt_string s, hlt_exception** excpt)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);
    return 0;
}

hlt_string hlt_regexp_string_span(hlt_regexp* re, const hlt_string s, hlt_exception** excpt)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);
    return hlt_string_from_asciiz("", excpt);
}

hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, const hlt_string s, hlt_exception** excpt)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);
    return 0;
}

// Bytes versions.
struct match_state { 
    hlt_bytes_pos begin;
    hlt_bytes_pos end;
    hlt_bytes_pos cur;
};

// Searches for the regexp at arbitrary starting positions and returns the
// first match. 
static jrx_accept_id _search_pattern(hlt_regexp* re, jrx_match_state* ms, 
                                     const hlt_bytes_pos begin, const hlt_bytes_pos end, 
                                     jrx_offset* so, jrx_offset* eo, hlt_exception** excpt)
{
    // We follow one of two strategies here:
    // 
    // (1) If the compilation was compiled with the ability to capture
    // subgroups, we have to use the more expensive standard matcher anyway.
    // In that case, the compilation didn't anchor the regexp (i.e,, it will
    // start with an implicit ".*") and we just need a single matching
    // process over all the data.
    // 
    // (2) If we compiled with REG_NOSUB, we iterate ourselves over all
    // possible starting positions so that even though using the more
    // efficinet minimal matcher, we can still get starting and end
    // positions. 
    
    // FIXME: In (2), we might be doing a bit more comparisions than with an
    // implicit .*, and the manual loop also adds a bit overhead. That seems
    // worth it but should reevaluate the trade-off later. 

    hlt_bytes_block block;
    jrx_assertion first = JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;
    jrx_assertion last;
    void* cookie;
    jrx_accept_id acc = 0;
    int8_t need_msdone = 0;
    hlt_bytes_size offset = 0;
    hlt_bytes_pos cur = begin;
    
    int8_t stdmatcher = ! (re->regexp.cflags & REG_NOSUB);
    
    while ( acc <= 0 && ! hlt_bytes_pos_eq(cur, end, excpt) ) {

        if ( need_msdone )
            jrx_match_state_done(ms);
        
        need_msdone = 1;
        cookie = 0;
        jrx_match_state_init(&re->regexp, offset, ms);
        
        while ( true ) {
            cookie = hlt_bytes_iterate_raw(&block, cookie, cur, end, excpt);
            
            if ( ! cookie )
                // Final chunk.
                last |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;
            
            int len = block.end - block.start;
            jrx_accept_id rc = jrx_regexec_partial(&re->regexp, (const char*)block.start, len, first, last, ms);

            if ( rc == 0 )
                // No match.
                break;
            
            if ( rc > 0 ) {
                // Match.
                acc = rc;
                
                if ( ! stdmatcher ) {
                    if ( so )
                        *so = offset;
                    if ( eo )
                        *eo = offset + ms->offset;
                }
                else if ( so || eo ) {
                    jrx_regmatch_t pmatch;
                    jrx_reggroups(&re->regexp, ms, 1, &pmatch);
                    
                    if ( so )
                        *so = pmatch.rm_so;
                    
                    if ( eo )
                        *eo = pmatch.rm_eo;
                }
            }
            
            if ( ! cookie ) {
                if ( rc < 0 && acc == 0 )
                    // At least one could match with more data.
                    acc = -1;
                break;
            }
        }
        
        if ( ! stdmatcher )
            // We compiled with an implicit ".*". 
            break;
        
        cur = hlt_bytes_pos_incr(cur, excpt);
        offset++;
        first = 0;
    }
    
    return acc;
}
    
int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }
    
    jrx_match_state ms;
    jrx_accept_id acc = _search_pattern(re, &ms, begin, end, 0, 0, excpt);
    jrx_match_state_done(&ms);
    return acc;
}
    
hlt_regexp_span_result hlt_regexp_bytes_span(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt)
{
    hlt_regexp_span_result result;
    
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return result;
    }

    jrx_offset so = -1;
    jrx_offset eo = -1;
    jrx_match_state ms;
    result.rc = _search_pattern(re, &ms, begin, end, &so, &eo, excpt);
    jrx_match_state_done(&ms);
    
    if ( result.rc > 0 ) {
        result.span.begin = hlt_bytes_pos_incr_by(begin, so, excpt);
        result.span.end = hlt_bytes_pos_incr_by(result.span.begin, eo - so, excpt);
    }
    
    else
        result.span.begin = result.span.end = hlt_bytes_end(excpt);
    
    return result;
}

extern const hlt_type_info hlt_type_info_tuple_iterator_bytes_iterator_bytes;

static inline void _set_group(hlt_vector* vec, hlt_bytes_pos begin, int i, jrx_offset so, jrx_offset eo, hlt_exception** excpt)
{
    hlt_regexp_range span;    
    span.begin = hlt_bytes_pos_incr_by(begin, so, excpt);
    span.end = hlt_bytes_pos_incr_by(begin, eo, excpt);
    hlt_vector_set(vec, i, &hlt_type_info_tuple_iterator_bytes_iterator_bytes, &span, excpt);
}

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt)
{
    if ( re->num != 1 ) {
        // No support for sets.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }

    hlt_regexp_range def_span;
    def_span.begin = def_span.end = hlt_bytes_end(excpt);

    hlt_vector* vec = hlt_vector_new(&hlt_type_info_tuple_iterator_bytes_iterator_bytes, &def_span, excpt);

    jrx_offset so = -1;
    jrx_offset eo = -1;
    jrx_match_state ms;
    int8_t rc = _search_pattern(re, &ms, begin, end, &so, &eo, excpt);

    if ( rc > 0 ) {
        _set_group(vec, begin, 0, so, eo, excpt);
        
        int num_groups = jrx_num_groups(&re->regexp);
        if ( num_groups < 1 )
            num_groups = 1;
        
        jrx_regmatch_t pmatch[num_groups];
        jrx_reggroups(&re->regexp, &ms, num_groups, pmatch);
        
        for ( int i = 1; i < num_groups; i++ ) {
            if ( pmatch[i].rm_so >= 0 )
                _set_group(vec, begin, i, pmatch[i].rm_so, pmatch[i].rm_eo, excpt);
        }
    }
    
    jrx_match_state_done(&ms);
    return vec;
}
    
