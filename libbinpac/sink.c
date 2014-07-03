
#include "sink.h"
#include "filter.h"

void binpac_dbg_print_data(binpac_sink* sink, hlt_bytes* data, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx);


typedef struct __parser_state {
    binpac_parser* parser;
    void* pobj;
    hlt_bytes* data;
    hlt_exception* resume;
    int disconnected;
    struct __parser_state* next;
} __parser_state;

struct binpac_sink {
    __hlt_gchdr __gch;                  // Header for garbage collection.
    __parser_state* head;               // List of parsing states.
    binpac_filter* filter;              // Potential filter attached.
    uint64_t size;                      // Number of bytes written so far.
};

__HLT_RTTI_GC_TYPE(binpac_sink, HLT_TYPE_BINPAC_SINK);

static void __unlink_state(binpac_sink* sink, __parser_state* state)
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

    GC_CLEAR(state->data, hlt_bytes);
    GC_CLEAR(state->resume, hlt_exception);
    GC_DTOR_GENERIC(&state->pobj, state->parser->type_info);
    state->pobj = 0;
    GC_CLEAR(state->parser, hlt_BinPACHilti_Parser);
    hlt_free(state);
}

static void __finish_parser(binpac_sink* sink, __parser_state* state, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( state->data && state->resume ) {
        hlt_bytes_freeze(state->data, 1, excpt, ctx);

        // TODO: Not sure if we need this wrapping with fiber/excpt here
        // (neither in main write function).
        hlt_fiber* saved_fiber = ctx->fiber;
        ctx->fiber = 0;
        hlt_exception* sink_excpt = 0;
        (state->parser->resume_func_sink)(state->resume, &sink_excpt, ctx);
        GC_CLEAR(state->resume, hlt_exception);
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
        // sink-unit-filter.pac2 tests for this but is currently disabled ...
        if ( sink_excpt && sink_excpt->type == &hlt_exception_yield ) {
            assert(false);
        }

        else
            *excpt = sink_excpt;
    }

    __unlink_state(sink, state);
}

void binpac_sink_dtor(hlt_type_info* ti, binpac_sink* sink)
{
    // TODO: This is not consistnely called because HILTI is actually using
    // its own type info for the dummy struct type. We should unify that, but
    // it's not clear how .. For now, we can't rely on this running.
#if 0
    while ( sink->head )
        __unlink_state(sink, sink->head);

    GC_CLEAR(sink->filter, binpac_filter);
#endif
}

binpac_sink* binpachilti_sink_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_sink* sink = GC_NEW(binpac_sink);
    sink->head = 0;
    sink->filter = 0;
    sink->size = 0;
    return sink;
}

void binpachilti_sink_connect(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _binpachilti_sink_connect_intern(sink, type, pobj, parser, 0, excpt, ctx);
}

void _binpachilti_sink_connect_intern(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __parser_state* state = hlt_malloc(sizeof(__parser_state));
    state->parser = parser;
    GC_CCTOR(state->parser, hlt_BinPACHilti_Parser);
    state->pobj = *pobj;
    GC_CCTOR_GENERIC(&state->pobj, type);
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

        DBG_LOG("binpac-sinks", "connected parser %s [%p] to sink %p for MIME type %s", r2, *pobj, sink, r1);

        hlt_free(r1);
        hlt_free(r2);
        GC_DTOR(s, hlt_string);
    }
    else {
        char* p = hlt_string_to_native(parser->name, excpt, ctx);
        DBG_LOG("binpac-sinks", "connected parser %s [%p] to sink %p", p, *pobj, sink);
        hlt_free(p);
    }
#endif

}

void binpachilti_sink_disconnect(binpac_sink* sink, const hlt_type_info* type, void** pobj, hlt_exception** excpt, hlt_execution_context* ctx)
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
    DBG_LOG("binpac-sinks", "disconnected parser %s [%p] from sink %p", p, *pobj, sink);
    hlt_free(p);
#endif

    // We don't delete the object here as we may be deep inside the parsing
    // when disconnecting is called.
    s->disconnected = 1;
}

void binpac_dbg_print_data(binpac_sink* sink, hlt_bytes* data, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx)
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

        hlt_iterator_bytes q = p;
        p = hlt_iterator_bytes_incr(p, excpt, ctx);
        GC_DTOR(q, hlt_iterator_bytes);
    }

    buffer[i] = '\0';

    const char* dots = hlt_iterator_bytes_eq(p, end, excpt, ctx) ? "" : " ...";

    if ( ! filter )
        DBG_LOG("binpac-sinks", "writing to sink %p: |%s%s|", sink, buffer, dots);
    else
        DBG_LOG("binpac-sinks", "    filtered by %s: |%s%s|", filter->def->name, buffer, dots);

    GC_DTOR(p, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);
#endif
}

void binpachilti_sink_write(binpac_sink* sink, hlt_bytes* data, void* user, hlt_exception** excpt, hlt_execution_context* ctx)
{
    DBG_LOG("binpac-sinks", "starting to write to sink %p", sink);

    // If the HILTI function that we'll call suspends, it will change the
    // yield/resume fields. We need to reset them when we leave this
    // function.

    hlt_fiber* saved_fiber = ctx->fiber;
    __hlt_thread_mgr_blockable* saved_blockable = ctx->blockable;

    ctx->fiber = 0;
    ctx->blockable = 0;

    binpac_dbg_print_data(sink, data, 0, excpt, ctx);

    GC_CCTOR(data, hlt_bytes);

    if ( sink->filter ) {
        hlt_bytes* decoded = binpachilti_filter_decode(sink->filter, data, excpt, ctx);

        if ( ! decoded )
            goto exit;

        GC_DTOR(data, hlt_bytes);
        data = decoded;
    }

    sink->size += hlt_bytes_len(data, excpt, ctx);

    if ( *excpt )
        goto exit;

    if ( ! sink->head ) {
        DBG_LOG("binpac-sinks", "done writing to sink %p, no parser connected", sink);
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
            DBG_LOG("binpac-sinks", "- start writing to sink %p for parser %p", sink, s->pobj);
            s->data = hlt_bytes_clone(data, excpt, ctx);

            if ( hlt_bytes_is_frozen(data, excpt, ctx) )
                hlt_bytes_freeze(s->data, 1, excpt, ctx);

            (s->parser->parse_func_sink)(s->pobj, s->data, user, &sink_excpt, ctx);
        }

        else {
            if ( ! s->resume ) {
                // Parsing has already finished for this parser. We ignore this here,
                // as otherwise we'd have to throw an exception and thus abort the
                // other parsers as well.
                s = s->next;
                continue;
            }

            DBG_LOG("binpac-sinks", "- resuming writing to sink %p for parser %p", sink, s->pobj);

            // Subsequent chunk, resume.
            hlt_bytes_append(s->data, data, excpt, ctx);

            if ( hlt_bytes_is_frozen(data, excpt, ctx) )
                hlt_bytes_freeze(s->data, 1, excpt, ctx);

            (s->parser->resume_func_sink)(s->resume, &sink_excpt, ctx);
            GC_CLEAR(s->resume, hlt_exception);
        }

        // Clear any left-over exceptions.
        __hlt_context_clear_exception(ctx);

        if ( sink_excpt && sink_excpt->type == &hlt_exception_yield ) {
            // Suspended.
            DBG_LOG("binpac-sinks", "- writing to sink %p suspended for parser %p", sink, s->pobj);
            GC_CCTOR(sink_excpt, hlt_exception);
            s->resume = sink_excpt;
            s = s->next;
            sink_excpt = 0;
        }

        else {
            DBG_LOG("binpac-sinks", "- writing to sink %p finished for parser %p", sink, s->pobj);
            __parser_state* next = s->next;
            // This guy is finished, remove.
            __unlink_state(sink, s);
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
                    GC_DTOR(ignore_excpt, hlt_exception);

                done = 0;
                break;
            }
        }
    }

exit:
    GC_DTOR(data, hlt_bytes);

    ctx->fiber = saved_fiber;
    ctx->blockable = saved_blockable;

    DBG_LOG("binpac-sinks", "done writing to sink %p", sink);
}

void binpachilti_sink_close(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx)
{
    DBG_LOG("binpac-sinks", "closing sink %p", sink);

    if ( sink->filter ) {
        binpachilti_filter_close(sink->filter, excpt, ctx);
        GC_CLEAR(sink->filter, binpac_filter);
    }

    while( sink->head )
        __finish_parser(sink, sink->head, excpt, ctx);

    DBG_LOG("binpac-sinks", "closed sink %p, disconnected all parsers", sink);
}

void binpachilti_sink_add_filter(binpac_sink* sink, hlt_enum ftype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_filter* old_filter = sink->filter;
    sink->filter = binpachilti_filter_add(sink->filter, ftype, excpt, ctx);
    GC_DTOR(old_filter, binpac_filter);

    DBG_LOG("binpac-sinks", "attached filter %s to sink %p", sink->filter->def->name, sink);
}

uint64_t binpachilti_sink_size(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return sink->size;
}
