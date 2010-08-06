// $Id$
//
// TODO: Need to register cleanup with GC.

#include <string.h>
#include <jrx.h>

#include "hilti.h"

struct hlt_regexp {
    int32_t num; // Number of patterns in set.
    hlt_string* patterns;
    hlt_regexp_flags flags;
    jrx_regex_t regexp;
};

static hlt_string_constant ASCII = { 5, "ascii" };
static hlt_string_constant EMPTY = { 12, "<no-pattern>" };
static hlt_string_constant PIPE = { 4, " | " };

hlt_regexp* hlt_regexp_new(const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_REGEXP);
    assert(type->num_params == 1);

    int64_t *flags = (int64_t*) &(type->type_params);

    hlt_regexp* re = hlt_gc_malloc_non_atomic(sizeof(hlt_regexp));
    re->num = 0;
    re->patterns = 0;
    re->flags = (hlt_regexp_flags)(*flags);
    return re;
}

static inline int _cflags(hlt_regexp_flags flags) 
{
    int cflags = REG_EXTENDED;
    if ( flags & HLT_REGEXP_NOSUB )
        cflags |= REG_NOSUB;
    
    return cflags | ((cflags & REG_NOSUB) ? REG_ANCHOR : 0);
}

static void _compile_one(hlt_regexp* re, hlt_string pattern, int idx, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_bytes* p = hlt_string_encode(pattern, &ASCII, excpt, ctx);
    if ( *excpt )
        return;

    hlt_bytes_size plen = hlt_bytes_len(p, excpt, ctx);
    const int8_t* praw = hlt_bytes_to_raw(p, excpt, ctx);
    
    if ( jrx_regset_add(&re->regexp, (const char*)praw, plen) != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return;
    }
    
    re->patterns[idx] = pattern;
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    re->num = 1;
    re->patterns = hlt_gc_malloc_non_atomic(sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _cflags(re->flags));
    _compile_one(re, pattern, 0, excpt, ctx);
    jrx_regset_finalize(&re->regexp);
}

void hlt_regexp_compile_set(hlt_regexp* re, hlt_list* patterns, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }
    
    re->num = hlt_list_size(patterns, excpt, ctx);
    re->patterns = hlt_gc_malloc_non_atomic(re->num * sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _cflags(re->flags));

    hlt_list_iter i = hlt_list_begin(patterns, excpt, ctx);
    hlt_list_iter end = hlt_list_end(patterns, excpt, ctx);
    int idx = 0;
    
    while ( ! hlt_list_iter_eq(i, end, excpt, ctx) ) {
        hlt_string* pattern = hlt_list_iter_deref(i, excpt, ctx);
        _compile_one(re, *pattern, idx, excpt, ctx);
        
         if ( *excpt )
            return;
        
        i = hlt_list_iter_incr(i, excpt, ctx);
        idx++;
    }
    
    jrx_regset_finalize(&re->regexp);
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_regexp* re = *((const hlt_regexp**)obj);
    
    if ( ! re->num )
        return &EMPTY;
    
    if ( re->num == 1 )
        return re->patterns[0];
    
    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);
    
    for ( int32_t idx = 0; idx < re->num; idx++ ) {
        if ( idx > 0 )
            s = hlt_string_concat(s, &PIPE, excpt, ctx);
        s = hlt_string_concat(s, re->patterns[idx], excpt, ctx);
    }
    
    return s;
}

// String versions (not implemented yet).

int32_t hlt_regexp_string_find(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);
    return 0;
}

hlt_string hlt_regexp_string_span(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0);
    return hlt_string_from_asciiz("", excpt, ctx);
}

hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
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
                                     jrx_offset* so, jrx_offset* eo,
                                     int do_anchor, int find_partial_matches,
                                      hlt_exception** excpt, hlt_execution_context* ctx)
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
    // positions. (Setting do_anchor to 1 prevents the iteration and will
    // only match right from the beginning. Note that this flag only works
    // with REG_NOSUB).
    // 
    // If find_partial_matches is 0, we don't report a match as long as more
    // input could still change the result (i.e., there are still DFA
    // transitions possible after processing the last bytes). In this case,
    // the function returns -1 as if there wasn't any match yet.
    
    // FIXME: In (2), we might be doing a bit more comparisions than with an
    // implicit .*, and the manual loop also adds a bit overhead. That seems
    // worth it but should reevaluate the trade-off later. 

    hlt_bytes_block block;
    jrx_assertion first = JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;
    jrx_assertion last;
    void* cookie = 0;
    jrx_accept_id acc = 0;
    int8_t need_msdone = 0;
    hlt_bytes_size offset = 0;
    hlt_bytes_pos cur = begin;
    int block_len = 0;
    
    int8_t stdmatcher = ! (re->regexp.cflags & REG_NOSUB);

    assert( (! do_anchor) || (re->regexp.cflags & REG_NOSUB));
    
    if ( hlt_bytes_pos_eq(cur, end, excpt, ctx) ) { 
        // Nothing to do, but still need to init the match state.
        jrx_match_state_init(&re->regexp, offset, ms);
        return -1;
    }
    
    while ( acc <= 0 && ! hlt_bytes_pos_eq(cur, end, excpt, ctx) ) {

        if ( need_msdone )
            jrx_match_state_done(ms);
        
        need_msdone = 1;
        cookie = 0;
        jrx_match_state_init(&re->regexp, offset, ms);
        
        while ( true ) {
            cookie = hlt_bytes_iterate_raw(&block, cookie, cur, end, excpt, ctx);
            
            if ( ! cookie )
                // Final chunk.
                last |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;
            
            block_len = block.end - block.start;
            int fpm = (! cookie) && find_partial_matches;
            jrx_accept_id rc = jrx_regexec_partial(&re->regexp, (const char*)block.start, block_len, first, last, ms, fpm);

            if ( rc == 0 )
                // No further match.
                return acc;
            
            if ( rc > 0 ) {
                // Match.
                acc = rc;
                
                if ( ! stdmatcher ) {
                    if ( so )
                        *so = offset;
                    if ( eo )
                        *eo = offset + ms->offset - 1;
                }
                else if ( so || eo ) {
                    jrx_regmatch_t pmatch;
                    jrx_reggroups(&re->regexp, ms, 1, &pmatch);
                    
                    if ( so )
                        *so = pmatch.rm_so;
                    
                    if ( eo )
                        *eo = pmatch.rm_eo;
                }
                
                return acc;
            }

            if ( ! cookie ) {
                if ( rc < 0 && acc == 0 )
                    // At least one could match with more data.
                    acc = -1;
                break;
            }
        }
        
        if ( stdmatcher || do_anchor )
            // We compiled with an implicit ".*", or are asked to anchor.
            break;
        
        cur = hlt_bytes_pos_incr(cur, excpt, ctx);
        offset++;
        first = 0;
    }

    return acc;
}
    
int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }
    
    jrx_match_state ms;
    jrx_accept_id acc = _search_pattern(re, &ms, begin, end, 0, 0, 0, 1, excpt, ctx);
    jrx_match_state_done(&ms);
    return acc;
}
    
hlt_regexp_span_result hlt_regexp_bytes_span(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp_span_result result;
    
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return result;
    }

    jrx_offset so = -1;
    jrx_offset eo = -1;
    jrx_match_state ms;
    result.rc = _search_pattern(re, &ms, begin, end, &so, &eo, 0, 1, excpt, ctx);
    jrx_match_state_done(&ms);
    
    if ( result.rc > 0 ) {
        result.span.begin = hlt_bytes_pos_incr_by(begin, so, excpt, ctx);
        result.span.end = hlt_bytes_pos_incr_by(result.span.begin, eo - so, excpt, ctx);
    }
    
    else
        result.span.begin = result.span.end = hlt_bytes_generic_end(excpt, ctx);
    
    return result;
}

extern const hlt_type_info hlt_type_info_tuple_iterator_bytes_iterator_bytes;

static inline void _set_group(hlt_vector* vec, hlt_bytes_pos begin, int i, jrx_offset so, jrx_offset eo, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp_range span;    
    span.begin = hlt_bytes_pos_incr_by(begin, so, excpt, ctx);
    span.end = hlt_bytes_pos_incr_by(begin, eo, excpt, ctx);
    hlt_vector_set(vec, i, &hlt_type_info_tuple_iterator_bytes_iterator_bytes, &span, excpt, ctx);
}

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 1 ) {
        // No support for sets.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }

    hlt_regexp_range def_span;
    def_span.begin = def_span.end = hlt_bytes_generic_end(excpt, ctx);

    hlt_vector* vec = hlt_vector_new(&hlt_type_info_tuple_iterator_bytes_iterator_bytes, &def_span, excpt, ctx);

    jrx_offset so = -1;
    jrx_offset eo = -1;
    jrx_match_state ms;
    int8_t rc = _search_pattern(re, &ms, begin, end, &so, &eo, 0, 1, excpt, ctx);

    if ( rc > 0 ) {
        _set_group(vec, begin, 0, so, eo, excpt, ctx);
        
        int num_groups = jrx_num_groups(&re->regexp);
        if ( num_groups < 1 )
            num_groups = 1;
        
        jrx_regmatch_t pmatch[num_groups];
        jrx_reggroups(&re->regexp, &ms, num_groups, pmatch);
        
        for ( int i = 1; i < num_groups; i++ ) {
            if ( pmatch[i].rm_so >= 0 )
                _set_group(vec, begin, i, pmatch[i].rm_so, pmatch[i].rm_eo, excpt, ctx);
        }
    }
    
    jrx_match_state_done(&ms);
    return vec;
}

hlt_regexp_match_token_result hlt_regexp_bytes_match_token(hlt_regexp* re, const hlt_bytes_pos begin, const hlt_bytes_pos end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        hlt_regexp_match_token_result dummy;
        return dummy;
    }
    
    if ( ! (re->regexp.cflags & REG_NOSUB) ) {
        // Must be a REG_NOSUB pattern.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        hlt_regexp_match_token_result dummy;
        return dummy;
    }

    jrx_match_state ms;
    jrx_offset eo;
    jrx_accept_id rc = _search_pattern(re, &ms, begin, end, 0, &eo, 1, 0, excpt, ctx);
    jrx_match_state_done(&ms);
    
    hlt_regexp_match_token_result result = { rc, (rc > 0 ? hlt_bytes_pos_incr_by(begin, eo, excpt, ctx) : begin) };        
    return result;
}
