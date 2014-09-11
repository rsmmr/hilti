
#include <string.h>
#include <jrx.h>

#include "regexp.h"
#include "string_.h"
#include "memory_.h"
#include "autogen/hilti-hlt.h"

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

static inline int _cflags(hlt_regexp_flags flags)
{
    int cflags = REG_EXTENDED | REG_LAZY;
    if ( flags & HLT_REGEXP_NOSUB )
        cflags |= REG_NOSUB;
    if ( flags & HLT_REGEXP_FIRST_MATCH )
        cflags |= REG_FIRST_MATCH;

    return cflags | ((cflags & REG_NOSUB) ? REG_ANCHOR : 0);
}

// patter not net ref'ed.
static void _compile_one(hlt_regexp* re, hlt_string pattern, int idx, int re_refed, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // FIXME: For now, the pattern must contain only ASCII characters.
    hlt_bytes* p = hlt_string_encode(pattern, Hilti_Charset_ASCII, excpt, ctx);
    if ( hlt_check_exception(excpt) )
        return;

    hlt_bytes_size plen = hlt_bytes_len(p, excpt, ctx);
    int8_t tmp[plen];
    int8_t* praw = hlt_bytes_to_raw(tmp, plen, p, excpt, ctx);
    assert(praw);

    if ( jrx_regset_add(&re->regexp, (const char*)praw, plen) != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, pattern, ctx);
        return;
    }

    if ( ! re_refed )
        GC_CCTOR(pattern, hlt_string, ctx);

    re->patterns[idx] = pattern;
}

void hlt_regexp_dtor(hlt_type_info* ti, hlt_regexp* re, hlt_execution_context* ctx)
{
    for ( int i = 0; i < re->num; i++ )
        GC_DTOR(re->patterns[i], hlt_string, ctx);

    hlt_free(re->patterns);

    if ( re->num > 0 )
        jrx_regfree(&re->regexp);
}

void hlt_match_token_state_dtor(hlt_type_info* ti, hlt_match_token_state* t, hlt_execution_context* ctx)
{
    if ( t->re )
        // Will be set to zero once match_state_done is called.
        jrx_match_state_done(&t->ms);

    GC_DTOR(t->re, hlt_regexp, ctx);
}

static inline void _hlt_regexp_init(hlt_regexp* re, hlt_regexp_flags flags, hlt_exception** excpt, hlt_execution_context* ctx)
{
    re->num = 0;
    re->patterns = 0;
    re->flags = flags;
}

hlt_regexp* hlt_regexp_new(hlt_regexp_flags flags, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp* re = GC_NEW(hlt_regexp, ctx);
    _hlt_regexp_init(re, flags, excpt, ctx);
    return re;
}

void* hlt_regexp_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return GC_NEW_REF(hlt_regexp, ctx);
}

void hlt_regexp_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp* src = *(hlt_regexp**)srcp;
    hlt_regexp* dst = *(hlt_regexp**)dstp;

    dst->num = src->num;
    dst->flags = src->flags;
    dst->patterns = hlt_malloc(src->num * sizeof(hlt_string));

    for ( int i = 0; i < src->num; i++ )
        __hlt_clone(&dst->patterns[i], &hlt_type_info_hlt_string, &src->patterns[i], cstate, excpt, ctx);

    // TODO: Figure out a way to reuse the compiled regexp. Need to make that
    // thread-safe though.

    jrx_regset_init(&dst->regexp, -1, _cflags(dst->flags));

    for ( int idx = 0; idx < dst->num; idx++ ) {
        hlt_string pattern = dst->patterns[idx];
        _compile_one(dst, pattern, idx, 1, excpt, ctx);
    }

    jrx_regset_finalize(&dst->regexp);
}

static void _hlt_regexp_new_from_regexp_init(hlt_regexp* dst, hlt_regexp* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    dst->flags = other->flags;
    dst->num = other->num;
    dst->patterns = hlt_malloc(dst->num * sizeof(hlt_string));
    jrx_regset_init(&dst->regexp, -1, _cflags(dst->flags));

    for ( int idx = 0; idx < other->num; idx++ ) {
        hlt_string pattern = other->patterns[idx];
        _compile_one(dst, pattern, idx, 0, excpt, ctx);

         if ( hlt_check_exception(excpt) )
            return;
    }

    jrx_regset_finalize(&dst->regexp);
}

hlt_regexp* hlt_regexp_new_from_regexp(hlt_regexp* other, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_regexp* re = GC_NEW(hlt_regexp, ctx);
    _hlt_regexp_new_from_regexp_init(re, other, excpt, ctx);
    return re;
}

void hlt_regexp_compile(hlt_regexp* re, const hlt_string pattern, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return;
    }

    re->num = 1;
    re->patterns = hlt_malloc(sizeof(hlt_string));
    jrx_regset_init(&re->regexp, -1, _cflags(re->flags));
    _compile_one(re, pattern, 0, 0, excpt, ctx);

    if ( hlt_check_exception(excpt) )
	return;

    jrx_regset_finalize(&re->regexp);
}

void hlt_regexp_compile_set(hlt_regexp* re, hlt_list* patterns, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 0 ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
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
        _compile_one(re, *pattern, idx, 0, excpt, ctx);

         if ( hlt_check_exception(excpt) )
            return;

        i = hlt_iterator_list_incr(i, excpt, ctx);
        idx++;
    }

    jrx_regset_finalize(&re->regexp);
}

hlt_string hlt_regexp_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const hlt_regexp* re = *((const hlt_regexp**)obj);

    if ( ! re )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    if ( ! re->num )
        return hlt_string_from_asciiz("<no pattern>", excpt, ctx);

    if ( re->num == 1 )
        return re->patterns[0];

    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);
    hlt_string pipe = hlt_string_from_asciiz(" | ", excpt, ctx);;
    hlt_string slash = hlt_string_from_asciiz("/", excpt, ctx);;

    for ( int32_t idx = 0; idx < re->num; idx++ ) {
        if ( idx > 0 )
            s = hlt_string_concat(s, pipe, excpt, ctx);

        s = hlt_string_concat(s, slash, excpt, ctx);
        s = hlt_string_concat(s, re->patterns[idx], excpt, ctx);
        s = hlt_string_concat(s, slash, excpt, ctx);
    }

    return s;
}

char* hlt_regexp_to_asciiz(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // Don't need type info nor seen.
    hlt_string p = hlt_regexp_to_string(0, &re, 0, 0, excpt, ctx);
    char* c = hlt_string_to_native(p, excpt, ctx);
    return c;
}

// String versions (not implemented yet).

int32_t hlt_regexp_string_find(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0, ctx);
    return 0;
}

hlt_string hlt_regexp_string_span(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0, ctx);
    return 0;
}

hlt_vector *hlt_regexp_string_groups(hlt_regexp* re, const hlt_string s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_not_implemented, 0, ctx);
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

    int8_t stdmatcher = ! (re->regexp.cflags & REG_NOSUB);

    assert( (! do_anchor) || (re->regexp.cflags & REG_NOSUB));

    if ( hlt_iterator_bytes_eq(cur, end, excpt, ctx) ) {
        // Nothing to do, but still need to init the match state.
        jrx_match_state_init(&re->regexp, offset, ms);
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

            if ( rc == 0 )
                // No further match.
                return acc;

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

        cur = hlt_iterator_bytes_incr(cur, excpt, ctx);

        offset++;
        first = 0;
    }

    return acc;
}

int32_t hlt_regexp_bytes_find(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
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
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
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
}

hlt_vector *hlt_regexp_bytes_groups(hlt_regexp* re, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( re->num != 1 ) {
        // No support for sets.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
        return 0;
    }

    hlt_regexp_range def_span;
    def_span.begin = hlt_bytes_generic_end(excpt, ctx);
    def_span.end = hlt_bytes_generic_end(excpt, ctx);

    hlt_vector* vec = hlt_vector_new(&hlt_type_info_hlt_tuple_iterator_bytes_iterator_bytes, &def_span, 0, excpt, ctx);

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

static hlt_match_token_state* _match_token_init(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_match_token_state* state = GC_NEW(hlt_match_token_state, ctx);
    GC_INIT(state->re, re, hlt_regexp, ctx);
    state->acc = 0;
    state->first = JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;
    jrx_match_state_init(&re->regexp, 0, &state->ms);

    return state;
}

extern void __hlt_bytes_normalize_iter(hlt_iterator_bytes* pos);

static int _match_token_advance(hlt_match_token_state* state,
                                hlt_iterator_bytes begin, hlt_iterator_bytes end,
                                int8_t final,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_bytes_normalize_iter(&begin);
    __hlt_bytes_normalize_iter(&end);

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

#ifdef _DEBUG_MATCHING
            fprintf(stderr, "%p feeding |", state);
            print_bytes_raw((const char*)block.start, block_len, excpt, ctx);
            fprintf(stderr, "|\n");
#endif

        rc = jrx_regexec_partial(&state->re->regexp, (const char*)block.start, block_len, state->first, last, &state->ms, (last != 0));

#ifdef _DEBUG_MATCHING
        fprintf(stderr, "%p rc=%d ms->offset=%d\n", state, rc, state->ms.offset);
#endif

        if ( rc == 0 )
            // No further match possible.
            return state->acc > 0 ? state->acc : 0;

        if ( rc > 0 ) {
#ifdef _DEBUG_MATCHING
                fprintf(stderr, "%p ms->offset=%d\n", state, state->ms.offset-1);
#endif

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
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    if ( ! (re->regexp.cflags & REG_NOSUB) ) {
        // Must be a REG_NOSUB pattern.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    int8_t is_frozen = hlt_iterator_bytes_is_frozen(begin, excpt, ctx);

    hlt_match_token_state* state = _match_token_init(re, excpt, ctx);
    jrx_accept_id rc = _match_token_advance(state, begin, end, is_frozen, excpt, ctx);
    jrx_offset eo;
    _match_token_finish(state, &eo, excpt, ctx);

    hlt_regexp_match_token result;
    result.rc = rc;

    if ( rc > 0 )
        result.end = hlt_iterator_bytes_incr_by(begin, eo, excpt, ctx);
    else
        result.end = begin;

    return result;
}

hlt_match_token_state* hlt_regexp_match_token_init(hlt_regexp* re, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! re->num ) {
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
        return 0;
    }

    if ( ! (re->regexp.cflags & REG_NOSUB) ) {
        // Must be a REG_NOSUB pattern.
        hlt_set_exception(excpt, &hlt_exception_pattern_error, 0, ctx);
        return 0;
    }

    return _match_token_init(re, excpt, ctx);
}

hlt_regexp_match_token hlt_regexp_bytes_match_token_advance(hlt_match_token_state* state, const hlt_iterator_bytes begin, const hlt_iterator_bytes end, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! state->re ) {
        // Already finished.
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        hlt_regexp_match_token dummy;
        return dummy;
    }

    int8_t is_frozen = hlt_iterator_bytes_is_frozen(begin, excpt, ctx);

    jrx_accept_id rc = _match_token_advance(state, begin, end, is_frozen, excpt, ctx);

    if ( rc >= 0 ) {
        GC_CLEAR(state->re, hlt_regexp, ctx); // Mark as done.

        jrx_offset eo;
        _match_token_finish(state, &eo, excpt, ctx);

        hlt_regexp_match_token result;
        result.rc = rc;

        if ( rc > 0 )
            result.end = hlt_iterator_bytes_incr_by(begin, eo, excpt, ctx);
        else
            result.end = end;

        return result;
    }

    hlt_regexp_match_token result = { rc, end };
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
    hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
    int8_t tmp[len]
    const int8_t* b2 = hlt_bytes_to_raw(tmp, len, b, excpt, ctx);
    assert(raw);
    print_bytes_raw((const char*)b2, len, excpt, ctx);
}

#endif
