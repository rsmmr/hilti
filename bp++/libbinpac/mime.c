
#include "mime.h"

static hlt_map* mtypes = 0;

typedef struct mime_parser {
    binpac_parser* parser;
    struct mime_parser* next;
} mime_parser;

extern const hlt_type_info hlt_type_info_bytes;

// We create a dummy typeinfo for the map's values.
static hlt_type_info type_info_mp = {
    0, sizeof(mime_parser*), "(mime_parser)", 0, 0, 0, 0, 0, 0, 0
};

static hlt_string_constant ASCII = { 5, "ascii" };

static void add_parser(hlt_bytes* mt, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mtypes ) {
        // Init hash.
        // We don't have type information for the map's value type, but that's fine,
        // the map implementation doesn't need it anyway. We just set it to null.
        mtypes = hlt_map_new(&hlt_type_info_bytes, &type_info_mp, 0, excpt, ctx);
    }

    mime_parser* mp = hlt_gc_malloc_non_atomic(sizeof(mime_parser));
    mime_parser** cmp = hlt_gc_malloc_non_atomic(sizeof(mime_parser*));
    hlt_bytes** cmt = hlt_gc_malloc_non_atomic(sizeof(hlt_bytes*));

    if ( ! (mp && cmp && cmt) ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return;
    }

    // Init the new list entry.
    mp->parser = parser;
    mp->next = 0;


    // Deep-copy the pointers, the map needs that.
    *cmt = mt;
    *cmp = mp;

    mime_parser** current = hlt_map_get_default(mtypes, &hlt_type_info_bytes, cmt, &type_info_mp, 0, excpt, ctx);

    if ( current )
        mp->next = *current;

    hlt_map_insert(mtypes, &hlt_type_info_bytes, cmt, &type_info_mp, cmp, excpt, ctx);

#ifdef DEBUG
    hlt_string s = hlt_string_decode(mt, &ASCII, excpt, ctx);

    const char* r1 = hlt_string_to_native(s, excpt, ctx);
    const char* r2 = hlt_string_to_native(parser->name, excpt, ctx);

    DBG_LOG("binpac-sinks", "MIME type %s registered for parser %s", r1, r2);
#endif
}

hlt_bytes* main_type(hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_pos i = hlt_bytes_find_byte(mtype, '/', excpt, ctx);

    if ( hlt_bytes_pos_eq(i, hlt_bytes_end(mtype, excpt, ctx), excpt, ctx) )
        return 0;
    else {
        i = hlt_bytes_pos_incr(i, excpt, ctx);
        return hlt_bytes_sub(hlt_bytes_begin(mtype, excpt, ctx), i, excpt, ctx);
    }
}

void binpac_mime_register_parser(binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! (parser->mime_types && parser->_new_func) )
         return;

    hlt_list_iter i = hlt_list_begin(parser->mime_types, excpt, ctx);
    hlt_list_iter end = hlt_list_end(parser->mime_types, excpt, ctx);

    while ( ! (hlt_list_iter_eq(i, end, excpt, ctx) || *excpt) ) {
        hlt_string mt = *(hlt_string*) hlt_list_iter_deref(i, excpt, ctx);
        hlt_bytes* b = hlt_string_encode(mt, &ASCII, excpt, ctx);

        hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
        const int8_t* raw = hlt_bytes_to_raw(b, excpt, ctx);

        // If it ends in '/*', register with just the main type.
        if ( len > 2 && raw[len-1] == '*' && raw[len-2] == '/' ) {
            hlt_bytes* main = main_type(b, excpt, ctx);
            assert(main);
            b = main;
        }

        // If it's the catch-all "*", add an empty string.
        if ( len == 1 && raw[0] == '*' )
            b = hlt_bytes_new(excpt, ctx);

        add_parser(b, parser, excpt, ctx);
        i = hlt_list_iter_incr(i, excpt, ctx);
    }
}

void binpac_mime_connect_by_string(binpac_sink* sink, hlt_string mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_string_encode(mtype, &ASCII, excpt, ctx);
    return binpac_mime_connect_by_bytes(sink, b, excpt, ctx);
}

static void connect_one(binpac_sink* sink, hlt_bytes* mtype, hlt_bytes* mtype_full, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! mtypes )
        return;

    mime_parser** current = hlt_map_get_default(mtypes, &hlt_type_info_bytes, &mtype, &type_info_mp, 0, excpt, ctx);

    if ( ! current )
        return;

    mime_parser* mp = *current;

    while ( mp ) {
        assert(mp->parser->_new_func);

        // The C stub for the ctor function may mess with our yield/resume
        // information. We need to restore that afterwards.
        hlt_continuation* saved_yield = ctx->yield;
        hlt_continuation* saved_resume = ctx->resume;

        void* pobj = mp->parser->_new_func(sink, mtype, excpt, ctx);
        assert(pobj);

        ctx->yield = saved_yield;
        ctx->resume = saved_resume;

        _binpac_sink_connect_intern(sink, 0, &pobj, mp->parser, mtype_full, excpt, ctx);

        mp = mp->next;
    }
}

void binpac_mime_connect_by_bytes(binpac_sink* sink, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    connect_one(sink, mtype, mtype, excpt, ctx);

    // Do a second check just for the main type.
    hlt_bytes* mtype2 = main_type(mtype, excpt, ctx);
    if ( mtype )
        connect_one(sink, mtype2, mtype, excpt, ctx);

    // Check for catch-alls with the empty string.
    connect_one(sink, hlt_bytes_new(excpt, ctx), mtype, excpt, ctx);
}
