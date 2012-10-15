
#include <string.h>
#include <jrx.h>

#include "regexp.h"
#include "string_.h"
#include "memory_.h"

struct __hlt_regexp {
    __hlt_gchdr __gchdr; // Header for memory management.
    int32_t num; // Number of patterns in set.
    hlt_string* patterns;
    hlt_regexp_flags flags;
    jrx_regex_t regexp;
};

struct __hlt_match_token_state {
    __hlt_gchdr __gchdr; // Header for memory management.
    hlt_regexp* re;
    jrx_match_state ms;
    jrx_accept_id acc;
    int first;
};

// #define _DEBUG_MATCHING

#ifdef _DEBUG_MATCHING
static void print_bytes_raw(const char* b2, hlt_bytes_size size, hlt_exception** excpt, hlt_execution_context* ctx);
static void print_bytes(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx);
#endif

void hlt_regexp_dtor(hlt_type_info* ti, hlt_regexp* re)
{
    for ( int i = 0; i < re->num; i++ )
        GC_DTOR(re->patterns[i], hlt_string);

    hlt_free(re->patterns);

    if ( re->num > 0 )
        jrx_regfree(&re->regexp);
}

void hlt_match_token_state_dtor(hlt_type_info* ti, hlt_match_token_state* t)
{
    if ( t->re )
        // Will be set to zero once match_state_done is called.
        jrx_match_state_done(&t->ms);

    GC_DTOR(t->re, hlt_regexp);
}

hlt_regexp* hlt_regexp_new(const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_REGEXP);
    assert(type->num_params == 1);

    int64_t *flags = (int64_t*) &(type->type_params);

    hlt_regexp* re = GC_NEW(hlt_regexp);
    re->num = 0;
    re->patterns = 0;
    re->flags = (hlt_regexp_flags)(*flags);
    return re;
}

static inline int _cflags(hlt_regexp_flags flags)
{
    int cflags = REG_EXTENDED | REG_LAZY;
    if ( flags & HLT_REGEXP_NOSUB )
        cflags |= REG_NOSUB;

    return cflags | ((cflags & REG_NOSUB) ? REG_ANCHOR : 0);
}

// patter not net ref'ed.
static void _compile_one(hlt_regexp* re, hlt_string pattern, int idx, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_string ascii = hlt_string_from_asciiz("ascii", excpt, ctx);
    hlt_bytes* p = hlt_string_encode(pattern, ascii, excpt, ctx);
    GC_DTOR(ascii, hlt_string);
    if ( *excpt )
        return;

    hlt_bytes_size plen = hlt_bytes_len(p, excpt, ctx);
    int8_t* praw = hlt_bytes_to_raw(p, excpt, ctx);

    GC_DTOR(p, hlt_bytes);

    if ( jrx_regset_add(&re->regexp, (const char*)praw, plen) != 0 ) {
        hlt_free(praw);
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return;
    }

    hlt_free(praw);

    GC_CCTOR(pattern, hlt_string);
    re->patterns[idx] = pattern;
}

hlt_regexp* hlt_regexp_new_from_regexp(const hlt_type_info* type, hlt_regexp* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_REGEXP);
    assert(type->num_params == 1);

    int64_t *flags = (int64_t*) &(type->type_params);

    hlt_regexp* re = GC_NEW(hlt_regexp);
    re->flags = (hlt_regexp_flags)(*flags);
    re->num = other->num;
    re->patterns = hlt_malloc(re->num * sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _cflags(re->flags));

    for ( int idx = 0; idx < other->num; idx++ ) {
        hlt_string pattern = other->patterns[idx];
        _compile_one(re, pattern, idx, excpt, ctx);

         if ( *excpt )
            return 0;
    }

    jrx_regset_finalize(&re->regexp);

    return re;
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        return;
    }

    re->num = 1;
    re->patterns = hlt_malloc(sizeof(hlt_string));
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
    re->patterns = hlt_malloc(re->num * sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _cflags(re->flags));

    hlt_iterator_list i = hlt_list_begin(patterns, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(patterns, excpt, ctx);
    int idx = 0;

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {
        hlt_string* pattern = hlt_iterator_list_deref(i, excpt, ctx);
        _compile_one(re, *pattern, idx, excpt, ctx);
        GC_DTOR(*pattern, hlt_string);

         if ( *excpt )
            return;

        hlt_iterator_list j = i;
        i = hlt_iterator_list_incr(i, excpt, ctx);
        GC_DTOR(j, hlt_iterator_list);
        idx++;
    }

    GC_DTOR(i, hlt_iterator_list);
    GC_DTOR(end, hlt_iterator_list);

    jrx_regset_finalize(&re->regexp);
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_regexp* re = *((const hlt_regexp**)obj);

    if ( ! re->num )
        return hlt_string_from_asciiz("<no pattern>", excpt, ctx);

    if ( re->num == 1 ) {
        GC_CCTOR(re->patterns[0], hlt_string);
        return re->patterns[0];
    }

    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);
    hlt_string pipe = hlt_string_from_asciiz(" | ", excpt, ctx);;
    hlt_string slash = hlt_string_from_asciiz("/", excpt, ctx);;

    for ( int32_t idx = 0; idx < re->num; idx++ ) {
        if ( idx > 0 ) {
            GC_CCTOR(pipe, hlt_string);
            s = hlt_string_concat_and_unref(s, pipe, excpt, ctx);
        }

        GC_CCTOR(slash, hlt_string);
        s = hlt_string_concat_and_unref(s, slash, excpt, ctx);

        GC_CCTOR(re->patterns[idx], hlt_string);
        s = hlt_string_concat_and_unref(s, re->patterns[idx], excpt, ctx);

        GC_CCTOR(slash, hlt_string);
        s = hlt_string_concat_and_unref(s, slash, excpt, ctx);
    }

    GC_DTOR(pipe, hlt_string);
    GC_DTOR(slash, hlt_string);
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

// Searches for the regexp at arbitrary starting positions and returns the
// first match.
//
// begin/end not yet ref'ed.
static jrx_accept_id _search_pattern(hlt_regexp* re, jrx_match_state* ms,
                                     const hlt_iterator_bytes begin, const hlt_iterator_bytes end,
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
    jrx_assertion last = 0;
    void* cookie = 0;
    jrx_accept_id acc = 0;
    int8_t need_msdone = 0;
    hlt_bytes_size offset = 0;
    int block_len = 0;
    int bytes_seen = 0;

    hlt_iterator_bytes cur = begin;
    GC_CCTOR(cur, hlt_iterator_bytes);

    int8_t stdmatcher = ! (re->regexp.cflags & REG_NOSUB);

    assert( (! do_anchor) || (re->regexp.cflags & REG_NOSUB));

    if ( hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {
        // Nothing to do, but still need to init the match state.
        jrx_match_state_init(&re->regexp, offset, ms);
        GC_DTOR(cur, hlt_iterator_bytes);
        return -1;
    }

    while ( acc <= 0 && ! hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {

        if ( need_msdone )
            jrx_match_state_done(ms);

        need_msdone = 1;
        cookie = 0;
        bytes_seen = 0;

        jrx_match_state_init(&re->regexp, offset, ms);

        while ( 1 ) {
            cookie = hlt_bytes_iterate_raw(&block, cookie, cur, end, excpt, ctx);

            if ( ! cookie )
                // Final chunk.
                last |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;

            block_len = block.end - block.start;
            int fpm = (! cookie) && find_partial_matches;

#ifdef _DEBUG_MATCHING
            fprintf(stderr, "feeding |");
            print_bytes_raw((const char*)block.start, block_len, excpt, ctx);
            fprintf(stderr, "|\n");
#endif
            jrx_accept_id rc = jrx_regexec_partial(&re->regexp, (const char*)block.start, block_len, first, last, ms, fpm);

#ifdef _DEBUG_MATCHING
            fprintf(stderr, "rc=%d ms->offset=%d\n", rc, ms->offset);
#endif

            if ( rc == 0 ) {
                // No further match.
                GC_DTOR(cur, hlt_iterator_bytes);
                return acc;
            }

            if ( rc > 0 ) {
                // Match.
                acc = rc;
#ifdef _DEBUG_MATCHING
                fprintf(stderr, "offset=%ld ms->offset=%d bytes_seen=%d eo=%p so=%p\n", offset, ms->offset-1, bytes_seen, eo, so);
#endif

                if ( ! stdmatcher ) {
                    if ( so )
                        *so = offset;
                    if ( eo ) {
                        // FIXME: The match_state intializes the offset with
                        // 1. Not sure why right now but changing that would
                        // probably break other things we adjust that here
                        // for the calculation.
                        *eo = offset + ms->offset - 1;
                    }
                }
                else if ( so || eo ) {
                    jrx_regmatch_t pmatch;
                    jrx_reggroups(&re->regexp, ms, 1, &pmatch);

                    if ( so )
                        *so = pmatch.rm_so;

                    if ( eo )
                        *eo = pmatch.rm_eo;
                }

                GC_DTOR(cur, hlt_iterator_bytes);
                return acc;
            }

            bytes_seen += block_len;

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

        hlt_iterator_bytes tmp = cur;
        cur = hlt_iterator_bytes_incr(cur, excpt, ctx);
        GC_DTOR(tmp, hlt_iterator_bytes);

        offset++;
        first = 0;
    }

    GC_DTOR(cur, hlt_iterator_bytes);
    return acc;
}

int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
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

hlt_regexp_span hlt_regexp_bytes_span(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp_span result;

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
        result.span.begin = hlt_iterator_bytes_incr_by(begin, so, excpt, ctx);
        result.span.end = hlt_iterator_bytes_incr_by(result.span.begin, eo - so, excpt, ctx);
    }

    else
        result.span.begin = result.span.end = hlt_bytes_generic_end(excpt, ctx);

    return result;
}

static inline void _set_group(hlt_vector* vec, hlt_iterator_bytes begin, int i, jrx_offset so, jrx_offset eo, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp_range span;
    span.begin = hlt_iterator_bytes_incr_by(begin, so, excpt, ctx);
    span.end = hlt_iterator_bytes_incr_by(begin, eo, excpt, ctx);
    hlt_vector_set(vec, i, &hlt_type_info_hlt_tuple_iterator_bytes_iterator_bytes, &span, excpt, ctx);
    GC_DTOR(span.begin, hlt_iterator_bytes);
    GC_DTOR(span.end, hlt_iterator_bytes);
}

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 1 ) {
        // No support for sets.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }

    hlt_regexp_range def_span;
    def_span.begin = hlt_bytes_generic_end(excpt, ctx);
    def_span.end = hlt_bytes_generic_end(excpt, ctx);

    hlt_vector* vec = hlt_vector_new(&hlt_type_info_hlt_tuple_iterator_bytes_iterator_bytes, &def_span, 0, excpt, ctx);

    GC_DTOR(def_span.begin, hlt_iterator_bytes);
    GC_DTOR(def_span.end, hlt_iterator_bytes);

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

static hlt_match_token_state* _match_token_init(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx_)
{
    hlt_match_token_state* state = GC_NEW(hlt_match_token_state);
    GC_INIT(state->re, re, hlt_regexp);
    state->acc = 0;
    state->first = JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;
    jrx_match_state_init(&re->regexp, 0, &state->ms);

    return state;
}

static int _match_token_advance(hlt_match_token_state* state,
                                hlt_iterator_bytes begin, const hlt_iterator_bytes end,
                                int8_t final,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_block block;
    void *cookie = 0;
    int last = 0;
    jrx_accept_id rc = 0;

    state->ms.offset = 1; // See below why 1.

    if ( hlt_iterator_bytes_eq(begin, end, excpt, ctx) )
        return -1;

    do {
        cookie = hlt_bytes_iterate_raw(&block, cookie, begin, end, excpt, ctx);

        if ( final && ! cookie )
            // Final chunk.
            last |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;

        int block_len = block.end - block.start;
        rc = jrx_regexec_partial(&state->re->regexp, (const char*)block.start, block_len, state->first, last, &state->ms, (last != 0));

        if ( rc == 0 ) {
            // No further match possible.
            return state->acc;
        }

        if ( rc > 0 ) {
            // Match found.
            state->acc = rc;
            return state->acc;
        }

    } while ( cookie );

    if ( rc < 0 && state->acc == 0 )
        // At least one could match with more data.
        state->acc = -1;

    return state->acc;
}

static void _match_token_finish(hlt_match_token_state* state, jrx_offset* eo,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(eo);

    if ( eo )
        // FIXME: The jrx match_state intializes the offset with
        // 1. Not sure why right now but changing that would
        // probably break other things we adjust that here
        // for the calculation.
        *eo = state->ms.offset - 1;

   jrx_match_state_done(&state->ms);
}

hlt_regexp_match_token hlt_regexp_bytes_match_token(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    if ( ! (re->regexp.cflags & REG_NOSUB) ) {
        // Must be a REG_NOSUB pattern.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    int8_t is_frozen = hlt_iterator_bytes_is_frozen(begin, excpt, ctx);

    hlt_match_token_state* state = _match_token_init(re, excpt, ctx);
    jrx_accept_id rc = _match_token_advance(state, begin, end, is_frozen, excpt, ctx);
    jrx_offset eo;
    _match_token_finish(state, &eo, excpt, ctx);

    GC_DTOR(state, hlt_match_token_state);

    hlt_regexp_match_token result;
    result.rc = rc;

    if ( rc > 0 )
        result.end = hlt_iterator_bytes_incr_by(begin, eo, excpt, ctx);
    else {
        result.end = begin;
        GC_CCTOR(result.end, hlt_iterator_bytes);
    }

    return result;
}

hlt_match_token_state* hlt_regexp_match_token_init(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }

    if ( ! (re->regexp.cflags & REG_NOSUB) ) {
        // Must be a REG_NOSUB pattern.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0);
        return 0;
    }

    return _match_token_init(re, excpt, ctx);
}

hlt_regexp_match_token hlt_regexp_bytes_match_token_advance(hlt_match_token_state* state, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! state->re ) {
        // Already finished.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    int8_t is_frozen = hlt_iterator_bytes_is_frozen(begin, excpt, ctx);

    jrx_accept_id rc = _match_token_advance(state, begin, end, is_frozen, excpt, ctx);

    if ( rc >= 0 ) {
        GC_CLEAR(state->re, hlt_regexp); // Mark as done.

        jrx_offset eo;
        _match_token_finish(state, &eo, excpt, ctx);

        hlt_regexp_match_token result;
        result.rc = rc;

        if ( rc > 0 )
            result.end = hlt_iterator_bytes_incr_by(begin, eo, excpt, ctx);
        else {
            result.end = end;
            GC_CCTOR(result.end, hlt_iterator_bytes);
        }

        return result;
    }

    hlt_regexp_match_token result = { rc, end };
    GC_CCTOR(result.end, hlt_iterator_bytes);
    return result;
}

#ifdef _DEBUG_MATCHING
static void print_bytes_raw(const char* b2, hlt_bytes_size size, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int x = 0;
    if ( size > 40 ) {
        x = 1;
        size = 40;
    }

    while ( size-- ) {
        int8_t c = *b2++;

        if ( isprint(c) )
            fputc((char)c, stderr);
        else
            fprintf(stderr, "\\x%02x", (unsigned int)c);
    }

    if ( x )
        fprintf(stderr, "...");

    fprintf(stderr, "|");
}

static void print_bytes(hlt_bytes* b, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const int8_t* b2 = hlt_bytes_to_raw(b, excpt, ctx);
    hlt_bytes_size size = hlt_bytes_len(b, excpt, ctx);
    print_bytes_raw((const char*)b2, size, excpt, ctx);
}

#endif
