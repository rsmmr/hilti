
#include "mime.h"
#include "globals.h"

typedef struct __mime_parser {
    __hlt_gchdr __gch;
    spicy_parser* parser;
    struct __mime_parser* next;
} __mime_parser;

__HLT_RTTI_GC_TYPE(__mime_parser, HLT_TYPE_SPICY_MIME_PARSER);

extern const hlt_type_info hlt_type_info_hlt_bytes;

void __mime_parser_dtor(hlt_type_info* ti, __mime_parser* mime_parser, hlt_execution_context* ctx)
{
    GC_CLEAR(mime_parser->parser, hlt_SpicyHilti_Parser, ctx);
    GC_CLEAR(mime_parser->next, __mime_parser, ctx);
}

static void __add_parser(hlt_bytes* mt, spicy_parser* parser, hlt_exception** excpt,
                         hlt_execution_context* ctx)
{
    __mime_parser* mp = GC_NEW_REF(__mime_parser, ctx);
    GC_INIT(mp->parser, parser, hlt_SpicyHilti_Parser, ctx);
    mp->next = 0;

    // Deep-copy the pointers, the map needs that.

    __mime_parser** cmp = hlt_malloc(sizeof(__mime_parser*));
    hlt_bytes** cmt = hlt_malloc(sizeof(hlt_bytes*));
    *cmt = mt;
    *cmp = mp;

    __mime_parser** current =
        hlt_map_get_default(__spicy_globals()->mime_types, &hlt_type_info_hlt_bytes, cmt,
                            &hlt_type_info___mime_parser, 0, excpt, ctx);

    if ( current ) {
        mp->next = *current;
        GC_CCTOR(mp->next, __mime_parser, ctx);
    }

    hlt_map_insert(__spicy_globals()->mime_types, &hlt_type_info_hlt_bytes, cmt,
                   &hlt_type_info___mime_parser, cmp, excpt, ctx);

    GC_DTOR(mp, __mime_parser, ctx);

    hlt_free(cmp);
    hlt_free(cmt);

#ifdef DEBUG
    hlt_string s = hlt_string_decode(mt, Hilti_Charset_ASCII, excpt, ctx);

    char* r1 = hlt_string_to_native(s, excpt, ctx);
    char* r2 = hlt_string_to_native(parser->name, excpt, ctx);

    DBG_LOG("spicy-sinks", "MIME type %s registered for parser %s", r1, r2);

    hlt_free(r1);
    hlt_free(r2);
#endif
}

static hlt_bytes* __main_type(hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_iterator_bytes i = hlt_bytes_find_byte(mtype, '/', excpt, ctx);
    hlt_iterator_bytes begin = hlt_bytes_begin(mtype, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(mtype, excpt, ctx);

    hlt_bytes* result = 0;

    if ( ! hlt_iterator_bytes_eq(i, end, excpt, ctx) ) {
        i = hlt_iterator_bytes_incr(i, excpt, ctx);
        result = hlt_bytes_sub(begin, i, excpt, ctx);
    }

    return result;
}

static void __connect_one(spicy_sink* sink, hlt_bytes* mtype, hlt_bytes* mtype_full,
                          int8_t try_mode, void* cookie, hlt_exception** excpt,
                          hlt_execution_context* ctx)
{
    if ( ! __spicy_globals()->mime_types )
        return;

    __mime_parser** current =
        hlt_map_get_default(__spicy_globals()->mime_types, &hlt_type_info_hlt_bytes, &mtype,
                            &hlt_type_info___mime_parser, 0, excpt, ctx);

    if ( ! current )
        return;

    __mime_parser* mparser = *current;
    for ( __mime_parser* mp = mparser; mp; mp = mp->next ) {
        assert(mp->parser->new_func);

        // The C stub for the ctor function may mess with our yield/resume
        // information. We need to restore that afterwards.
        hlt_fiber* saved_fiber = ctx->fiber;
        __hlt_thread_mgr_blockable* saved_blockable = ctx->blockable;

        // We may trigger a safepoint so make sure we refcount our locoal.
        GC_CCTOR(sink, spicy_sink, ctx);
        GC_CCTOR(mtype, hlt_bytes, ctx);
        GC_CCTOR(mtype_full, hlt_bytes, ctx);

        void* pobj = mp->parser->new_func(sink, mtype, try_mode, cookie, excpt, ctx);
        assert(pobj);

        GC_DTOR(sink, spicy_sink, ctx);
        GC_DTOR(mtype, hlt_bytes, ctx);
        GC_DTOR(mtype_full, hlt_bytes, ctx);

        ctx->fiber = saved_fiber;
        ctx->blockable = saved_blockable;

        _spicyhilti_sink_connect_intern(sink, mp->parser->type_info, &pobj, mp->parser, mtype_full,
                                         excpt, ctx);
    }
}

void spicyhilti_mime_register_parser(spicy_parser* parser, hlt_exception** excpt,
                                      hlt_execution_context* ctx)
{
    if ( ! (parser->mime_types && parser->new_func) )
        return;

    hlt_iterator_list i = hlt_list_begin(parser->mime_types, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parser->mime_types, excpt, ctx);

    while ( ! (hlt_iterator_list_eq(i, end, excpt, ctx) || *excpt) ) {
        hlt_string mt = *(hlt_string*)hlt_iterator_list_deref(i, excpt, ctx);
        hlt_bytes* b = hlt_string_encode(mt, Hilti_Charset_ASCII, excpt, ctx);

        hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
        int8_t tmp[len];
        int8_t* raw = hlt_bytes_to_raw(tmp, len, b, excpt, ctx);
        assert(raw);

        // If it ends in '/*', register with just the main type.
        if ( len > 2 && raw[len - 1] == '*' && raw[len - 2] == '/' ) {
            hlt_bytes* main = __main_type(b, excpt, ctx);
            assert(main);
            b = main;
        }

        // If it's the catch-all "*", add an empty string.
        if ( len == 1 && raw[0] == '*' ) {
            b = hlt_bytes_new(excpt, ctx);
        }

        __add_parser(b, parser, excpt, ctx);

        i = hlt_iterator_list_incr(i, excpt, ctx);
    }
}

void spicyhilti_sink_connect_mimetype_string(spicy_sink* sink, hlt_string mtype, int8_t try_mode,
                                              void* cookie, hlt_exception** excpt,
                                              hlt_execution_context* ctx)
{
    hlt_bytes* b = hlt_string_encode(mtype, Hilti_Charset_ASCII, excpt, ctx);
    spicyhilti_sink_connect_mimetype_bytes(sink, b, 0, cookie, excpt, ctx);
}

void spicyhilti_sink_connect_mimetype_bytes(spicy_sink* sink, hlt_bytes* mtype, int8_t try_mode,
                                             void* cookie, hlt_exception** excpt,
                                             hlt_execution_context* ctx)
{
    __connect_one(sink, mtype, mtype, try_mode, cookie, excpt, ctx);

    // Do a second check just for the main type.
    hlt_bytes* mtype2 = __main_type(mtype, excpt, ctx);
    if ( mtype2 )
        __connect_one(sink, mtype2, mtype, try_mode, cookie, excpt, ctx);

    // Check for catch-alls with the empty string.
    hlt_bytes* mtype3 = hlt_bytes_new(excpt, ctx);
    __connect_one(sink, mtype3, mtype, try_mode, cookie, excpt, ctx);
}
