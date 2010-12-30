
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

binpac_sink* binpacintern_sink_new(hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_sink* sink = hlt_gc_malloc_non_atomic(sizeof(binpac_sink));
    if ( ! sink ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    sink->head = 0;

    return sink;
}

void binpacintern_sink_connect(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
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
    state->next = 0;

    // Link into the list.
    if ( sink->head )
        sink->head->next = state;
    else
        sink->head = state;
}

void binpacintern_sink_disconnect(binpac_sink* sink, const hlt_type_info* type, void** pobj, hlt_exception** excpt, hlt_execution_context* ctx)
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
}

void binpacintern_sink_write(binpac_sink* sink, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx)
{
    parser_state* prev = 0;
    parser_state* s = sink->head;

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

void binpacintern_sink_close(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx)
{
    for ( parser_state* s = sink->head; s; s = s->next )
        finish_parsing(s, excpt, ctx);

    sink->head = 0;
}
