
#include <ctype.h>

#include <autogen/spicy-hlt.h>

#include "exceptions.h"
#include "filter.h"
#include "sink.h"

// XXX Needed?
// void spicy_dbg_print_data(spicy_sink* sink, hlt_bytes* data, spicy_filter* filter,
// hlt_exception** excpt, hlt_execution_context* ctx);

typedef struct __parser_state {
    spicy_parser* parser;
    void* pobj;
    hlt_bytes* data;
    hlt_exception* resume;
    int disconnected;
    struct __parser_state* next;
} __parser_state;

typedef struct __chunk {
    struct __chunk* next; // Next block. Has ownership.
    struct __chunk* prev; // Previous block.
    uint64_t rseq;        // Sequence number of first byte.
    uint64_t rupper;      // Sequence number of last byte + 1.
    hlt_bytes* data;      // Data at +1.
} __chunk;

struct spicy_sink {
    __hlt_gchdr __gch;    // Header for garbage collection.
    __parser_state* head; // List of parsing states.
    spicy_filter* filter; // Potential filter attached.
    uint64_t size;        // Number of bytes written so far. TODO: Still needed? XXX

    // Reassembly state.
    int64_t policy;             // Reassembly policy of type SpicyHilti::ReassemblyPolicy.
    int8_t auto_trim;           // True if automatic trimming is enabled.
    uint64_t initial_seq;       // Initial sequence number.
    uint64_t cur_rseq;          // Sequence of last delivered byte + 1 (i.e., seq of next)
    uint64_t last_reassem_rseq; // Sequence of last byte reassembled and delivered + 1.
    uint64_t trim_rseq;         // Sequence of last byte trimmed so far + 1.
    __chunk* first_chunk;       // First not yet reassembled chunk. Has ownership.
    __chunk* last_chunk;        // Last not yet reassembled chunk.
};

__HLT_RTTI_GC_TYPE(spicy_sink, HLT_TYPE_SPICY_SINK);

void spicy_dbg_deliver(spicy_sink* sink, hlt_bytes* data, spicy_filter* filter,
                       hlt_exception** excpt, hlt_execution_context* ctx);
void spicy_dbg_reassembler(spicy_sink* sink, const char* msg, hlt_bytes* data, uint64_t seq,
                           int64_t len, hlt_exception** excpt, hlt_execution_context* ctx);
void spicy_dbg_reassembler_buffer(spicy_sink* sink, const char* msg, hlt_exception** excpt,
                                  hlt_execution_context* ctx);

static void __cctor_state(__parser_state* state, hlt_execution_context* ctx)
{
    for ( ; state; state = state->next ) {
        GC_CCTOR(state->parser, hlt_SpicyHilti_Parser, ctx);
        GC_CCTOR_GENERIC(&state->pobj, state->parser->type_info, ctx);
        GC_CCTOR(state->data, hlt_bytes, ctx);
        GC_CCTOR(state->resume, hlt_exception, ctx);
    }
}

static void __dtor_state(__parser_state* state, hlt_execution_context* ctx)
{
    for ( ; state; state = state->next ) {
        GC_DTOR(state->parser, hlt_SpicyHilti_Parser, ctx);
        GC_DTOR_GENERIC(&state->pobj, state->parser->type_info, ctx);
        GC_DTOR(state->data, hlt_bytes, ctx);
        GC_DTOR(state->resume, hlt_exception, ctx);
    }
}

static void __unlink_state(spicy_sink* sink, __parser_state* state, hlt_execution_context* ctx)
{
    __parser_state* prev = 0;
    __parser_state* s = sink->head;

    while ( s ) {
        if ( s == state )
            break;

        prev = s;
        s = s->next;
    }

    assert(s);

    if ( prev )
        prev->next = state->next;

    else
        sink->head = state->next;

    GC_CLEAR(state->data, hlt_bytes, ctx);
    GC_CLEAR(state->resume, hlt_exception, ctx);
    GC_DTOR_GENERIC(&state->pobj, state->parser->type_info, ctx);
    state->pobj = 0;
    GC_CLEAR(state->parser, hlt_SpicyHilti_Parser, ctx);
    hlt_free(state);
}

static void __finish_parser(spicy_sink* sink, __parser_state* state, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    if ( state->data && state->resume ) {
        hlt_bytes_freeze(state->data, 1, excpt, ctx);

        // TODO: Not sure if we need this wrapping with fiber/excpt here
        // (neither in main write function).
        hlt_fiber* saved_fiber = ctx->fiber;
        ctx->fiber = 0;
        hlt_exception* sink_excpt = 0;

        // We may trigger a safepoint so make sure we refcount our locoal.
        GC_CCTOR(sink, spicy_sink, ctx);
        __cctor_state(state, ctx);

        (state->parser->resume_func_sink)(state->resume, &sink_excpt, ctx);

        GC_DTOR(sink, spicy_sink, ctx);
        __dtor_state(state, ctx);

        GC_CLEAR(state->resume, hlt_exception, ctx);
        __hlt_context_clear_exception(ctx);
        ctx->fiber = saved_fiber;

        // TODO: Not quite sure why the yield exception can occur here, but
        // we ignore it.
        //
        // Update: Actually I think I do: when two separate filters (like
        // first for a sink and then for the unit connected to it) filter the
        // same data, the frozen flags doesn't get propagated.
        //
        // We forbid that for now and hope it won't be used ...
        //
        // sink-unit-filter.spicy tests for this but is currently disabled ...
        if ( sink_excpt && sink_excpt->type == &hlt_exception_yield ) {
            assert(false);
        }

        else if ( sink_excpt && sink_excpt->type == &spicy_exception_parserdisabled ) {
            // Disabled, nothing to do.
            DBG_LOG("spicy-sinks", "- final writing to sink %p disabled for parser %p", sink,
                    state->pobj);
            GC_DTOR(sink_excpt, hlt_exception, ctx);
        }

        else
            *excpt = sink_excpt;
    }

    __unlink_state(sink, state, ctx);
}

static int8_t __check_seq(spicy_sink* sink, uint64_t seq, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( seq >= sink->initial_seq )
        return 1;

    spicyhilti_sink_close(sink, user, excpt, ctx);

    hlt_string msg = hlt_string_from_asciiz("invalid sequence number", excpt, ctx);
    hlt_set_exception(excpt, &spicy_exception_valueerror, msg, ctx);

    return 0;
}

static void __report_gap(spicy_sink* sink, uint64_t rseq, uint64_t len, void* user,
                         hlt_exception** excpt, hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "reporting gap in sink %p at rseq %" PRIu64, sink, rseq);

    for ( __parser_state* s = sink->head; s; s = s->next ) {
        if ( s->parser->hook_gap )
            (*s->parser->hook_gap)(s->pobj, user, rseq + sink->initial_seq, len, excpt, ctx);
    }
}

static void __report_skip(spicy_sink* sink, uint64_t rseq, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "reporting skip in sink %p to rseq %" PRIu64, sink, rseq);

    for ( __parser_state* s = sink->head; s; s = s->next ) {
        if ( s->parser->hook_skip )
            (*s->parser->hook_skip)(s->pobj, user, rseq + sink->initial_seq, excpt, ctx);
    }
}

static void __report_overlap(spicy_sink* sink, hlt_bytes* old_data, hlt_bytes* new_data,
                             uint64_t rseq, void* user, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "reporting overlap in sink %p at rseq %" PRIu64, sink, rseq);

    if ( ! old_data )
        old_data =
            hlt_bytes_new_from_data((int8_t*)"<unavailable>", sizeof("<unavailable>"), excpt, ctx);

    if ( ! new_data )
        new_data =
            hlt_bytes_new_from_data((int8_t*)"<unavailable>", sizeof("<unavailable>"), excpt, ctx);

    for ( __parser_state* s = sink->head; s; s = s->next ) {
        if ( s->parser->hook_overlap )
            (*s->parser->hook_overlap)(s->pobj, user, rseq + sink->initial_seq, old_data, new_data,
                                       excpt, ctx);
    }
}

static void __report_undelivered(spicy_sink* sink, uint64_t rseq, hlt_bytes* b, void* user,
                                 hlt_exception** excpt, hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "reporting undelivered in sink %p at rseq %" PRIu64, sink, rseq);

    for ( __parser_state* s = sink->head; s; s = s->next ) {
        if ( s->parser->hook_undelivered )
            (*s->parser->hook_undelivered)(s->pobj, user, rseq + sink->initial_seq, b, excpt, ctx);
    }
}

static void __report_undelivered_up_to(spicy_sink* sink, uint64_t rupper, void* user,
                                       hlt_exception** excpt, hlt_execution_context* ctx)
{
    for ( __chunk* c = sink->first_chunk; c && c->rseq < rupper; c = c->next ) {
        if ( ! c->data )
            continue;

        hlt_bytes* b;

        if ( c->rupper <= rupper )
            b = c->data;

        else {
            hlt_iterator_bytes i = hlt_bytes_begin(c->data, excpt, ctx);
            hlt_iterator_bytes j = hlt_iterator_bytes_incr_by(i, (c->rupper - rupper), excpt, ctx);
            b = hlt_bytes_sub(i, j, excpt, ctx);
        }

        __report_undelivered(sink, c->rseq, b, user, excpt, ctx);
    }
}

static __chunk* __new_chunk(hlt_bytes* data, uint64_t rseq, uint64_t len, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    __chunk* c = hlt_malloc(sizeof(__chunk));
    c->next = 0;
    c->prev = 0;
    c->rseq = rseq;
    c->rupper = rseq + len;
    GC_ASSIGN(c->data, data, hlt_bytes, ctx);
    return c;
}

static void __link_chunk(spicy_sink* sink, __chunk* prev, __chunk* c, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    if ( prev ) {
        if ( prev->next )
            prev->next->prev = c;
        else
            sink->last_chunk = c;

        c->next = prev->next;
        prev->next = c;
        c->prev = prev;
    }

    else {
        // Insert at head.
        c->next = sink->first_chunk;

        if ( sink->first_chunk )
            sink->first_chunk->prev = c;
        else
            sink->last_chunk = c;

        sink->first_chunk = c;
    }
}

static void __unlink_chunk(spicy_sink* sink, __chunk* c, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    if ( c->next )
        c->next->prev = c->prev;
    else
        sink->last_chunk = c->prev;

    if ( c->prev )
        c->prev->next = c->next;
    else
        sink->first_chunk = c->next;

    c->next = 0;
    c->prev = 0;
}

static void __delete_chunk(__chunk* c, hlt_exception** excpt, hlt_execution_context* ctx)
{
    GC_DTOR(c->data, hlt_bytes, ctx);
    hlt_free(c);
}

static void __trim(spicy_sink* sink, uint64_t rseq, void* user, hlt_exception** excpt,
                   hlt_execution_context* ctx)
{
#ifdef DEBUG
    if ( rseq != UINT64_MAX ) {
        DBG_LOG("spicy-sinks", "trimming sink %p to rseq %" PRIu64, sink, rseq);
    }

    else {
        DBG_LOG("spicy-sinks", "trimming sink %p to eod", sink);
    }
#endif

    __chunk* c = sink->first_chunk;

    while ( c ) {
        if ( c->rupper > rseq )
            break;

        if ( c->rupper < sink->cur_rseq && c->data )
            __report_undelivered(sink, c->rseq, c->data, user, excpt, ctx);

        __chunk* n = c->next;
        __unlink_chunk(sink, c, excpt, ctx);
        __delete_chunk(c, excpt, ctx);
        c = n;
    }

    sink->trim_rseq = rseq;
}

static void __try_deliver(spicy_sink* sink, __chunk* c, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx);

static void __skip(spicy_sink* sink, uint64_t rseq, void* user, hlt_exception** excpt,
                   hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "skipping sink %p to rseq %" PRIu64, sink, rseq);

    __report_undelivered_up_to(sink, rseq, user, excpt, ctx);

    if ( sink->auto_trim )
        __trim(sink, rseq, user, excpt, ctx);

    sink->cur_rseq = rseq;
    sink->last_reassem_rseq = rseq;

    __report_skip(sink, rseq, user, excpt, ctx);
    __try_deliver(sink, sink->first_chunk, user, excpt, ctx);
}

static int8_t __deliver_chunk(spicy_sink* sink, hlt_bytes* data, uint64_t rseq, uint64_t rupper,
                              void* user, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! data ) {
        // A gap.
        DBG_LOG("spicy-sinks", "hit gap with sink %p at rseq %" PRIu64, sink, rseq);

        if ( sink->cur_rseq != rupper ) {
            __report_gap(sink, rseq, (rupper - rseq), user, excpt, ctx);
            sink->cur_rseq = rupper;

            if ( sink->auto_trim )
                // We trim just up to the gap here, excluding the gap itself. This
                // will prevent future data beyond the gap from being delivered until
                // we explicitly skip over it.
                __trim(sink, rseq, user, excpt, ctx);
        }

        return 0;
    }

    DBG_LOG("spicy-sinks", "starting to deliver to sink %p at rseq %" PRIu64, sink, rseq);

    // If the HILTI function that we'll call suspends, it will change the
    // yield/resume fields. We need to reset them when we leave this
    // function.

    hlt_fiber* saved_fiber = ctx->fiber;
    __hlt_thread_mgr_blockable* saved_blockable = ctx->blockable;

    ctx->fiber = 0;
    ctx->blockable = 0;

    spicy_dbg_deliver(sink, data, 0, excpt, ctx);

    if ( sink->filter ) {
        hlt_bytes* decoded = spicyhilti_filter_decode(sink->filter, data, excpt, ctx); // &noref!

        if ( ! decoded )
            goto exit;

        data = decoded;
    }

    sink->size += hlt_bytes_len(data, excpt, ctx);

    if ( *excpt )
        goto exit;

    if ( ! sink->head ) {
        DBG_LOG("spicy-sinks", "done delivering to sink %p, no parser connected", sink);
        goto exit;
    }

    // data at +1 here.

    __parser_state* s = sink->head;

    // Now pass it onto parsers.
    while ( s ) {
        if ( s->disconnected )
            continue;

        hlt_exception* sink_excpt = 0;

        if ( ! s->data ) {
            // First chunk.
            DBG_LOG("spicy-sinks", "- start delivering to sink %p for parser %p", sink, s->pobj);
            s->data = hlt_bytes_copy(data, excpt, ctx);
            GC_CCTOR(s->data, hlt_bytes, ctx);

            if ( hlt_bytes_is_frozen(data, excpt, ctx) )
                hlt_bytes_freeze(s->data, 1, excpt, ctx);

            // We may trigger a safepoint so make sure we refcount our locoal.
            GC_CCTOR(sink, spicy_sink, ctx);
            GC_CCTOR(data, hlt_bytes, ctx);
            __cctor_state(s, ctx);

            (s->parser->parse_func_sink)(s->pobj, s->data, user, &sink_excpt, ctx);

            GC_DTOR(sink, spicy_sink, ctx);
            GC_DTOR(data, hlt_bytes, ctx);
            __dtor_state(s, ctx);
        }

        else {
            if ( ! s->resume ) {
                // Parsing has already finished for this parser. We ignore this here,
                // as otherwise we'd have to throw an exception and thus abort the
                // other parsers as well.
                s = s->next;
                continue;
            }

            DBG_LOG("spicy-sinks", "- resuming delivering to sink %p for parser %p", sink, s->pobj);

            // Subsequent chunk, resume.
            hlt_bytes_append(s->data, data, excpt, ctx);

            if ( hlt_bytes_is_frozen(data, excpt, ctx) )
                hlt_bytes_freeze(s->data, 1, excpt, ctx);

            // We may trigger a safepoint so make sure we refcount our locoal.
            GC_CCTOR(sink, spicy_sink, ctx);
            GC_CCTOR(data, hlt_bytes, ctx);
            __cctor_state(s, ctx);

            (s->parser->resume_func_sink)(s->resume, &sink_excpt, ctx);

            GC_DTOR(sink, spicy_sink, ctx);
            GC_DTOR(data, hlt_bytes, ctx);
            __dtor_state(s, ctx);

            GC_CLEAR(s->resume, hlt_exception, ctx);
        }

        // Clear any left-over exceptions.
        __hlt_context_clear_exception(ctx);

        if ( sink_excpt && sink_excpt->type == &hlt_exception_yield ) {
            // Suspended.
            DBG_LOG("spicy-sinks", "- delivering to sink %p suspended for parser %p", sink,
                    s->pobj);
            GC_CCTOR(sink_excpt, hlt_exception, ctx);
            s->resume = sink_excpt;
            s = s->next;
            sink_excpt = 0;
        }

        else if ( sink_excpt && sink_excpt->type == &spicy_exception_parserdisabled ) {
            // Disabled
            DBG_LOG("spicy-sinks", "- writing to sink %p disabled for parser %p", sink, s->pobj);
            GC_DTOR(sink_excpt, hlt_exception, ctx);
            sink_excpt = 0;
            __parser_state* next = s->next;
            // This guy is finished, remove.
            __unlink_state(sink, s, ctx);
            s = next;
        }

        else {
            DBG_LOG("spicy-sinks", "- delivering to sink %p finished for parser %p", sink, s->pobj);
            __parser_state* next = s->next;
            // This guy is finished, remove.
            __unlink_state(sink, s, ctx);
            s = next;
        }

        if ( sink_excpt ) {
            // Error, abort.
            *excpt = sink_excpt;
            goto exit;
        }
    }

    // Savely delete disconnected parsers.

    int done = 0;

    while ( ! done ) {
        done = 1;
        for ( __parser_state* s = sink->head; s; s = s->next ) {
            if ( s->disconnected ) {
                // We ignore parse error triggered by the disconnect.
                hlt_exception* ignore_excpt = 0;
                __finish_parser(sink, s, &ignore_excpt, ctx);

                if ( ignore_excpt )
                    GC_DTOR(ignore_excpt, hlt_exception, ctx);

                done = 0;
                break;
            }
        }
    }

exit:
    ctx->fiber = saved_fiber;
    ctx->blockable = saved_blockable;

    sink->cur_rseq = rupper;
    sink->last_reassem_rseq = rupper;

    if ( sink->auto_trim )
        __trim(sink, rupper, user, excpt, ctx);

    DBG_LOG("spicy-sinks", "done delivering to sink %p", sink);

    return 1;
}

static void __try_deliver(spicy_sink* sink, __chunk* c, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    // Note that a new block may include both some old stuff and some new
    // stuff. __add_and_checkk() will have split the new stuff off into its
    // own block(s), but in the following loop we have to take care not to
    // deliver already-delivered data.

    while ( c && c->rseq <= sink->last_reassem_rseq ) {
        __chunk* next = c->next;

        if ( c->rseq == sink->last_reassem_rseq ) {
            // New stuff.
            sink->last_reassem_rseq += (c->rupper - c->rseq);
            if ( ! __deliver_chunk(sink, c->data, c->rseq, c->rupper, user, excpt, ctx) )
                // Hit gap.
                break;
        }

        c = next;
    }
}

static __chunk* __add_and_check(spicy_sink* sink, __chunk* b, uint64_t rseq, uint64_t rupper,
                                hlt_bytes* data, void* user, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    assert(sink->first_chunk);
    assert(sink->last_chunk);

    // Special check for the common case of appending to the end.
    if ( rseq == sink->last_chunk->rupper ) {
        __chunk* c = __new_chunk(data, rseq, rupper - rseq, excpt, ctx);
        __link_chunk(sink, sink->last_chunk, c, excpt, ctx);
        return c;
    }

    // Find the first block that doesn't come completely before the new data.
    while ( b->next && b->rupper <= rseq )
        b = b->next;

    if ( b->rupper <= rseq ) {
        // b is the last block, and it comes completely before the new block.
        __chunk* c = __new_chunk(data, rseq, rupper - rseq, excpt, ctx);
        __link_chunk(sink, b, c, excpt, ctx);
        return c;
    }

    if ( rupper <= b->rseq ) {
        // The new block comes completely before b.
        __chunk* c = __new_chunk(data, rseq, rupper - rseq, excpt, ctx);
        __link_chunk(sink, b->prev, c, excpt, ctx);
        return c;
    }

    __chunk* new_b = 0;

    // The blocks overlap, complain.

    if ( rseq < b->rseq ) {
        // The new block has a prefix that comes before b.
        int64_t prefix_len = b->rseq - rseq;

        if ( data ) {
            hlt_iterator_bytes begin = hlt_bytes_begin(data, excpt, ctx);
            hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);
            hlt_iterator_bytes i = hlt_iterator_bytes_incr_by(begin, prefix_len, excpt, ctx);
            hlt_bytes* sub = hlt_bytes_sub(begin, i, excpt, ctx);
            new_b = __new_chunk(sub, rseq, prefix_len, excpt, ctx);
            __link_chunk(sink, b->prev, new_b, excpt, ctx);

            data = hlt_bytes_sub(i, end, excpt, ctx);
        }

        rseq += prefix_len;
    }

    else
        new_b = b;

    uint64_t overlap_start = rseq;
    int64_t new_b_len = rupper - rseq;
    int64_t b_len = (b->rupper - overlap_start);
    int64_t overlap_len = (new_b_len < b_len ? new_b_len : b_len);

    hlt_bytes* old_data = 0;
    hlt_bytes* new_data = 0;

    if ( b->data ) {
        hlt_iterator_bytes i = hlt_bytes_begin(b->data, excpt, ctx);
        i = hlt_iterator_bytes_incr_by(i, (overlap_start - b->rseq), excpt, ctx);
        hlt_iterator_bytes j = hlt_iterator_bytes_incr_by(i, overlap_len, excpt, ctx);
        old_data = hlt_bytes_sub(i, j, excpt, ctx);
    }

    if ( data ) {
        hlt_iterator_bytes i = hlt_bytes_begin(data, excpt, ctx);
        hlt_iterator_bytes j = hlt_iterator_bytes_incr_by(i, overlap_len, excpt, ctx);
        new_data = hlt_bytes_sub(i, j, excpt, ctx);
    }

    __report_overlap(sink, old_data, new_data, overlap_start, user, excpt, ctx);

    if ( overlap_len < new_b_len ) {
        // Recurse to resolve remainder of the new data.
        hlt_iterator_bytes i = hlt_bytes_begin(data, excpt, ctx);
        hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);
        i = hlt_iterator_bytes_incr_by(i, overlap_len, excpt, ctx);
        data = hlt_bytes_sub(i, end, excpt, ctx);
        rseq += overlap_len;

        if ( new_b == b )
            new_b = __add_and_check(sink, b, rseq, rupper, data, user, excpt, ctx);
        else
            (void)__add_and_check(sink, b, rseq, rupper, data, user, excpt, ctx);
    }

    return new_b;
}

// data==null signals a gap.
static void __new_block(spicy_sink* sink, hlt_bytes* data, uint64_t rseq, hlt_bytes_size len,
                        void* user, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( len == 0 )
        // Nothing to do.
        return;

    // Fast-path: if it's right at the end of the input stream, and we
    // haven't anything buffered, just pass on.
    if ( rseq == sink->cur_rseq && ! sink->first_chunk ) {
        spicy_dbg_reassembler(sink, "fastpath block", data, rseq, len, excpt, ctx);
        __deliver_chunk(sink, data, rseq, rseq + len, user, excpt, ctx);
        return;
    }

    spicy_dbg_reassembler(sink, "buffering block", data, rseq, len, excpt, ctx);

    uint64_t rupper_rseq = rseq + len;

    if ( rupper_rseq <= sink->trim_rseq )
        // Old data, don't do any work for it.
        goto exit;

    if ( rseq < sink->trim_rseq ) {
        // Partially old data, just keep the good stuff.
        int64_t amount_old = sink->trim_rseq - rseq;
        rseq += amount_old;

        if ( data ) {
            hlt_iterator_bytes begin = hlt_bytes_begin(data, excpt, ctx);
            hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);
            begin = hlt_iterator_bytes_incr_by(begin, amount_old, excpt, ctx);
            data = hlt_bytes_sub(begin, end, excpt, ctx);
        }
    }

    __chunk* c = 0;

    if ( ! sink->first_chunk ) {
        c = __new_chunk(data, rseq, len, excpt, ctx);
        sink->first_chunk = c;
        sink->last_chunk = c;
    }

    else
        c = __add_and_check(sink, sink->first_chunk, rseq, rupper_rseq, data, user, excpt, ctx);

// See if we have data in order now to deliver.

#if 0
    spicy_dbg_reassembler(sink, "AFTER add_and_check", c->data, c->rseq, c->rupper - c->rseq, excpt, ctx);
    spicy_dbg_reassembler_buffer(sink, "buffer", excpt, ctx);
#endif

    if ( c->rseq > sink->last_reassem_rseq || c->rupper <= sink->last_reassem_rseq )
        goto exit;

    // We've filled a leading hole. Deliver as much as possible.
    __try_deliver(sink, c, user, excpt, ctx);

exit:
    spicy_dbg_reassembler_buffer(sink, "buffer", excpt, ctx);
}

void spicy_sink_dtor(hlt_type_info* ti, spicy_sink* sink, hlt_execution_context* ctx)
{
// TODO: This is not consistnely called because HILTI is actually using
// its own type info for the dummy struct type. We should unify that, but
// it's not clear how .. For now, we can't rely on this running.
#if 0
    while ( sink->head )
        __unlink_state(sink, sink->head);

    GC_CLEAR(sink->filter, spicy_filter, ctx);

    __trim(sink, UINT64_MAX, user, excpt, ctx);
    assert(! sink->first_chunk);
    assert(! sink->last_chunk);
#endif
}

spicy_sink* spicyhilti_sink_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    spicy_sink* sink = GC_NEW(spicy_sink, ctx);
    sink->head = 0;
    sink->filter = 0;
    sink->size = 0;
    sink->auto_trim = 1;
    sink->initial_seq = 0;
    sink->policy = hlt_enum_value(Spicy_ReassemblyPolicy_First, excpt, ctx);
    sink->cur_rseq = 0;
    sink->last_reassem_rseq = 0;
    sink->trim_rseq = 0;
    sink->first_chunk = 0;
    sink->last_chunk = 0;
    return sink;
}

void spicyhilti_sink_set_initial_sequence_number(spicy_sink* sink, uint64_t initial_seq, void* user,
                                                 hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( sink->cur_rseq || sink->first_chunk ) {
        spicyhilti_sink_close(sink, user, excpt, ctx);
        hlt_string msg =
            hlt_string_from_asciiz("sink cannot change initial sequence number retrospectively",
                                   excpt, ctx);
        hlt_set_exception(excpt, &spicy_exception_valueerror, msg, ctx);
    }

    sink->initial_seq = initial_seq;
}

void spicyhilti_sink_set_policy(spicy_sink* sink, int64_t policy, void* user, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    if ( policy == hlt_enum_value(Spicy_ReassemblyPolicy_First, excpt, ctx) )
        return;

    spicyhilti_sink_close(sink, user, excpt, ctx);
    hlt_string msg = hlt_string_from_asciiz("sink reassembly policy not supported yet", excpt, ctx);
    hlt_set_exception(excpt, &spicy_exception_notimplemented, msg, ctx);
}

void spicyhilti_sink_set_auto_trim(spicy_sink* sink, int8_t enable, void* user,
                                   hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( enable )
        return;

    spicyhilti_sink_close(sink, user, excpt, ctx);
    hlt_string msg =
        hlt_string_from_asciiz("disabling sink auto-trim not yet supported", excpt, ctx);
    hlt_set_exception(excpt, &spicy_exception_notimplemented, msg, ctx);
}

void spicyhilti_sink_connect(spicy_sink* sink, const hlt_type_info* type, void** pobj,
                             spicy_parser* parser, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    _spicyhilti_sink_connect_intern(sink, type, pobj, parser, 0, excpt, ctx);
}

void _spicyhilti_sink_connect_intern(spicy_sink* sink, const hlt_type_info* type, void** pobj,
                                     spicy_parser* parser, hlt_bytes* mtype, hlt_exception** excpt,
                                     hlt_execution_context* ctx)
{
    __parser_state* state = hlt_malloc(sizeof(__parser_state));
    state->parser = parser;
    GC_CCTOR(state->parser, hlt_SpicyHilti_Parser, ctx);
    state->pobj = *pobj;
    GC_CCTOR_GENERIC(&state->pobj, type, ctx);
    state->data = 0;
    state->resume = 0;
    state->disconnected = 0;
    state->next = sink->head;

    sink->head = state;

#ifdef DEBUG
    if ( mtype ) {
        hlt_string s = hlt_string_decode(mtype, Hilti_Charset_ASCII, excpt, ctx);
        char* r1 = hlt_string_to_native(s, excpt, ctx);
        char* r2 = hlt_string_to_native(parser->name, excpt, ctx);

        DBG_LOG("spicy-sinks", "connected parser %s [%p] to sink %p for MIME type %s", r2, *pobj,
                sink, r1);

        hlt_free(r1);
        hlt_free(r2);
    }
    else {
        char* p = hlt_string_to_native(parser->name, excpt, ctx);
        DBG_LOG("spicy-sinks", "connected parser %s [%p] to sink %p", p, *pobj, sink);
        hlt_free(p);
    }
#endif
}

void spicyhilti_sink_disconnect(spicy_sink* sink, const hlt_type_info* type, void** pobj,
                                hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! sink )
        return;

    __parser_state* s = 0;

    for ( s = sink->head; s; s = s->next ) {
        if ( s->pobj == *pobj )
            break;
    }

    if ( ! s )
        // Not found, ignore.
        return;

#ifdef DEBUG
    char* p = hlt_string_to_native(s->parser->name, excpt, ctx);
    DBG_LOG("spicy-sinks", "disconnected parser %s [%p] from sink %p", p, *pobj, sink);
    hlt_free(p);
#endif

    // We don't delete the object here as we may be deep inside the parsing
    // when disconnecting is called.
    s->disconnected = 1;
}

void spicy_dbg_deliver(spicy_sink* sink, hlt_bytes* data, spicy_filter* filter,
                       hlt_exception** excpt, hlt_execution_context* ctx)
{
#ifdef DEBUG
    // Log data with non-printable characters escaped and output trimmed if
    // too long.
    int8_t buffer[50];
    int i = 0;

    hlt_iterator_bytes p = hlt_bytes_begin(data, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

    while ( ! hlt_iterator_bytes_eq(p, end, excpt, ctx) ) {
        int8_t c = hlt_iterator_bytes_deref(p, excpt, ctx);

        if ( isprint(c) )
            buffer[i++] = c;
        else {
            if ( i < sizeof(buffer) - 5 )
                i += snprintf((char*)(buffer + i), 5, "\\x%02x", c);
            else
                break;
        }

        if ( i >= sizeof(buffer) - 1 )
            break;

        p = hlt_iterator_bytes_incr(p, excpt, ctx);
    }

    buffer[i] = '\0';

    const char* dots = hlt_iterator_bytes_eq(p, end, excpt, ctx) ? "" : " ...";

    hlt_bytes_size len = hlt_bytes_len(data, excpt, ctx);

    if ( ! filter )
        DBG_LOG("spicy-sinks", "delivering to sink %p: |%s%s| (%" PRIu64 " bytes)", sink, buffer,
                dots, len);
    else
        DBG_LOG("spicy-sinks", "    filtered by %s: |%s%s| (%" PRIu64 " bytes)", filter->def->name,
                buffer, dots, len);

#endif
}

void spicy_dbg_reassembler_buffer(spicy_sink* sink, const char* msg, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
#ifdef DEBUG
    if ( ! sink->first_chunk ) {
        DBG_LOG("spicy-sinks", "reassembler/%p: no data buffered", sink);
        return;
    }

    char buffer[128];
    int i = 0;

    DBG_LOG("spicy-sinks",
            "reassembler/%p: %s ("
            "cur_rseq=%" PRIu64
            " "
            "last_reassem_rseq=%" PRIu64
            " "
            "trim_rseq=%" PRIu64 ")",
            sink, msg, sink->cur_rseq, sink->last_reassem_rseq, sink->trim_rseq);

    for ( __chunk* c = sink->first_chunk; c; c = c->next ) {
        snprintf(buffer, sizeof(buffer), "* chunk %d:", i);
        spicy_dbg_reassembler(sink, buffer, c->data, c->rseq, (c->rupper - c->rseq), excpt, ctx);
        i++;
    }
#endif
}

void spicy_dbg_reassembler(spicy_sink* sink, const char* msg, hlt_bytes* data, uint64_t seq,
                           int64_t len, hlt_exception** excpt, hlt_execution_context* ctx)
{
#ifdef DEBUG
    // Log data with non-printable characters escaped and output trimmed if
    // too long.
    const char* dots = 0;
    int8_t buffer[50];
    buffer[0] = '\0';
    hlt_bytes_size dlen = 0;

    if ( data ) {
        int i = 0;

        hlt_iterator_bytes p = hlt_bytes_begin(data, excpt, ctx);
        hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

        while ( ! hlt_iterator_bytes_eq(p, end, excpt, ctx) ) {
            int8_t c = hlt_iterator_bytes_deref(p, excpt, ctx);

            if ( isprint(c) )
                buffer[i++] = c;
            else {
                if ( i < sizeof(buffer) - 5 )
                    i += snprintf((char*)(buffer + i), 5, "\\x%02x", c);
                else
                    break;
            }

            if ( i >= sizeof(buffer) - 1 )
                break;

            p = hlt_iterator_bytes_incr(p, excpt, ctx);
        }

        buffer[i] = '\0';

        dlen = hlt_bytes_len(data, excpt, ctx);
        dots = hlt_iterator_bytes_eq(p, end, excpt, ctx) ? "" : " ...";
    }

    if ( data ) {
        if ( len >= 0 )
            DBG_LOG("spicy-sinks", "reassembler/%p: %s seq=% " PRIu64 " upper=%" PRIu64
                                   " |%s%s| (%" PRIu64 " bytes)",
                    sink, msg, seq, seq + len, buffer, dots, dlen);
        else
            DBG_LOG("spicy-sinks", "reassembler/%p: %s seq=% " PRIu64 " |%s%s| (%" PRIu64 " bytes)",
                    sink, msg, seq, buffer, dots, dlen);
    }

    else {
        if ( len >= 0 )
            DBG_LOG("spicy-sinks", "reassembler/%p: %s seq=%" PRIu64 " upper=%" PRIu64 " <GAP>",
                    sink, msg, seq, seq + len);
        else
            DBG_LOG("spicy-sinks", "reassembler/%p: %s seq=%" PRIu64 " <GAP>", sink, msg, seq);
    }
#endif
}

void spicyhilti_sink_append(spicy_sink* sink, hlt_bytes* data, void* user, hlt_exception** excpt,
                            hlt_execution_context* ctx)
{
    hlt_bytes_size len = hlt_bytes_len(data, excpt, ctx);
    __new_block(sink, data, sink->cur_rseq, len, user, excpt, ctx);
}

void spicyhilti_sink_write(spicy_sink* sink, hlt_bytes* data, uint64_t seq, void* user,
                           hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __check_seq(sink, seq, user, excpt, ctx) )
        return;

    hlt_bytes_size len = hlt_bytes_len(data, excpt, ctx);
    __new_block(sink, data, (seq - sink->initial_seq), len, user, excpt, ctx);
}

void spicyhilti_sink_write_custom_length(spicy_sink* sink, hlt_bytes* data, uint64_t seq,
                                         uint64_t len, void* user, hlt_exception** excpt,
                                         hlt_execution_context* ctx)
{
    if ( ! __check_seq(sink, seq, user, excpt, ctx) )
        return;

    __new_block(sink, data, (seq - sink->initial_seq), len, user, excpt, ctx);
}

void spicyhilti_sink_gap(spicy_sink* sink, uint64_t seq, uint64_t len, void* user,
                         hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __check_seq(sink, seq, user, excpt, ctx) )
        return;

    __new_block(sink, 0, (seq - sink->initial_seq), len, user, excpt, ctx);
}

void spicyhilti_sink_trim(spicy_sink* sink, uint64_t seq, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! __check_seq(sink, seq, user, excpt, ctx) )
        return;

    __trim(sink, (seq - sink->initial_seq), user, excpt, ctx);
    spicy_dbg_reassembler_buffer(sink, "buffer after trim", excpt, ctx);
}

void spicyhilti_sink_skip(spicy_sink* sink, uint64_t seq, void* user, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! __check_seq(sink, seq, user, excpt, ctx) )
        return;

    __skip(sink, (seq - sink->initial_seq), user, excpt, ctx);
    spicy_dbg_reassembler_buffer(sink, "buffer after skip", excpt, ctx);
}

void spicyhilti_sink_close(spicy_sink* sink, void* user, hlt_exception** excpt,
                           hlt_execution_context* ctx)
{
    DBG_LOG("spicy-sinks", "closing sink %p", sink);

    if ( sink->filter ) {
        spicyhilti_filter_close(sink->filter, excpt, ctx);
        GC_CLEAR(sink->filter, spicy_filter, ctx);
    }

    while ( sink->head )
        __finish_parser(sink, sink->head, excpt, ctx);

    __trim(sink, UINT64_MAX, user, excpt, ctx);
    assert(! sink->first_chunk);
    assert(! sink->last_chunk);

    sink->cur_rseq = 0;
    sink->last_reassem_rseq = 0;
    sink->trim_rseq = 0;

    DBG_LOG("spicy-sinks", "closed sink %p, disconnected all parsers", sink);
}

void spicyhilti_sink_add_filter(spicy_sink* sink, hlt_enum ftype, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    spicy_filter* old_filter = sink->filter;
    sink->filter = spicyhilti_filter_add(sink->filter, ftype, excpt, ctx);
    GC_CCTOR(sink->filter, spicy_filter, ctx);
    GC_DTOR(old_filter, spicy_filter, ctx);

    DBG_LOG("spicy-sinks", "attached filter %s to sink %p", sink->filter->def->name, sink);
}

uint64_t spicyhilti_sink_size(spicy_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return sink->size;
}

uint64_t spicyhilti_sink_sequence(spicy_sink* sink, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    // XXX
    return sink->cur_rseq + sink->initial_seq;
}
