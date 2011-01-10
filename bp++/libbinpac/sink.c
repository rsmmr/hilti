
#include "sink.h"

typedef struct parser_state {
    binpac_parser* parser;
    void* pobj;
    hlt_bytes* data;
    hlt_exception* resume;
    struct parser_state* next;
} parser_state;

struct binpac_sink {
    parser_state* head;
    binpac_filter* filter;
};

static void finish_parsing(parser_state* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! s->data )
        return;

    if ( ! s->resume )
        return;

    hlt_bytes_freeze(s->data, 1, excpt, ctx);
    (s->parser->_resume_func_sink)(s->resume, excpt, ctx);

    assert ( (! (*excpt)) || (*excpt)->type != &hlt_exception_yield);

    s->data = 0;
    s->pobj = 0;
    s->resume = 0;
}

binpac_sink* binpac_sink_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_sink* sink = hlt_gc_malloc_non_atomic(sizeof(binpac_sink));
    if ( ! sink ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    sink->head = 0;
    sink->filter = 0;

    return sink;
}

void binpac_sink_connect(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _binpac_sink_connect_intern(sink, type, pobj, parser, 0, excpt, ctx);
}

static hlt_string_constant ASCII = { 5, "ascii" };

void _binpac_sink_connect_intern(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    parser_state* state = hlt_gc_malloc_non_atomic(sizeof(parser_state));
    if ( ! state ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }

    state->parser = parser;
    state->pobj = *pobj;
    state->data = 0;
    state->resume = 0;

    // Link into the list.
    state->next = sink->head;
    sink->head = state;

#ifdef DEBUG
    if ( mtype ) {
        hlt_string s = hlt_string_decode(mtype, &ASCII, excpt, ctx);
        const char* r1 = hlt_string_to_native(s, excpt, ctx);
        const char* r2 = hlt_string_to_native(parser->name, excpt, ctx);

        DBG_LOG("binpac-sinks", "connected parser %s [%p] to sink %p for MIME type %s", r2, *pobj, sink, r1);
    }
    else {
        const char* p = hlt_string_to_native(parser->name, excpt, ctx);
        DBG_LOG("binpac-sinks", "connected parser %s [%p] to sink %p", p, *pobj, sink);
    }
#endif

}

void binpac_sink_disconnect(binpac_sink* sink, const hlt_type_info* type, void** pobj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    parser_state* prev = 0;
    parser_state* s = 0;
    for ( s = sink->head; s; prev = s, s = s->next ) {
        if ( s->pobj == *pobj )
            break;
    }

    if ( ! s )
        // Not found, ignore.
        return;

    finish_parsing(s, excpt, ctx);

    // Unlink.
    if ( prev )
        prev->next = s->next;
    else
        sink->head = sink->head->next;

#ifdef DEBUG
    const char* p = hlt_string_to_native(s->parser->name, excpt, ctx);
    DBG_LOG("binpac-sinks", "disconnected parser %s [%p] from sink %p", p, *pobj, sink);
#endif

}

void binpac_dbg_print_data(binpac_sink* sink, hlt_bytes* data, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx)
{
#ifdef DEBUG
    // Log data with non-printable characters escaped and output trimmed if
    // too long.
    int8_t buffer[50];
    int i = 0;

    for ( hlt_bytes_pos p = hlt_bytes_begin(data, excpt, ctx);
          ! hlt_bytes_pos_eq(p, hlt_bytes_end(data, excpt, ctx), excpt, ctx);
          p = hlt_bytes_pos_incr(p, excpt, ctx) ) {

        int8_t c = hlt_bytes_pos_deref(p, excpt, ctx);

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
    }

    if ( i < sizeof(buffer) )
        buffer[i] = '\0';

    const char* dots = hlt_bytes_pos_eq(p, hlt_bytes_end(data, excpt, ctx), excpt, ctx) ? "" : " ...";

    if ( ! filter )
        DBG_LOG("binpac-sinks", "writing to sink %p: |%s%s|", sink, buffer, dots);
    else
        DBG_LOG("binpac-sinks", "    filtered by %s: |%s%s|", filter->name, buffer, dots);
#endif
}

void binpac_sink_write(binpac_sink* sink, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_dbg_print_data(sink, data, 0, excpt, ctx);

    parser_state* prev = 0;
    parser_state* s = sink->head;

    if ( sink->filter ) {
        hlt_bytes* decoded = binpac_filter_decode(sink->filter, data, excpt, ctx);

        if ( ! decoded )
            return;

        if ( hlt_bytes_is_frozen(data, excpt, ctx) )
            // No more data going to come.
            hlt_bytes_freeze(decoded, 1, excpt, ctx);

        data = decoded;
    }

    // Now pass it onto parsers.
    while ( s ) {

        if ( ! s->data ) {
            // First chunk.
            s->data = hlt_bytes_copy(data, excpt, ctx);
            hlt_bytes_pos begin = hlt_bytes_begin(s->data, excpt, ctx);
            (s->parser->_parse_func_sink)(s->pobj, begin, 0, excpt, ctx);
        }

        else {
            if ( ! s->resume ) {
                // Parsing has already finished for this parser. We ignore this here,
                // as otherwise we'd have to throw an expception and thus abort the
                // other parsers as well.
                prev = s;
                s = s->next;
                continue;
            }

            // Subsequent chunk, resume.
            hlt_bytes_append(s->data, data, excpt, ctx);
            (s->parser->_resume_func_sink)(s->resume, excpt, ctx);
        }

        if ( *excpt && (*excpt)->type == &hlt_exception_yield ) {
            // Suspended.
            s->resume = *excpt;
            *excpt = 0;

            prev = s;
            s = s->next;
        }

        else {
            // This guy is finished, remove.
            if ( prev )
                prev->next = s->next;
            else
                sink->head = sink->head->next;

            s = s->next;
        }

        if ( *excpt )
            // Error, abort.
            return;
    }
}

void binpac_sink_close(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx)
{
    for ( parser_state* s = sink->head; s; s = s->next )
        finish_parsing(s, excpt, ctx);

    if ( sink->filter )
        binpac_filter_close(sink->filter, excpt, ctx);

    sink->head = 0;

#ifdef DEBUG
    DBG_LOG("binpac-sinks", "closed sink %p, disconnected all parsers", sink);
#endif
}

void binpac_sink_filter(binpac_sink* sink, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx)
{
    sink->filter = binpac_filter_add(sink->filter, filter, excpt, ctx);

#ifdef DEBUG
    DBG_LOG("binpac-sinks", "attached filter %s to sink %p", filter->name, sink);
#endif
}
