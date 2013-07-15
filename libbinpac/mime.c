
#include "mime.h"
#include "globals.h"

typedef struct __mime_parser {
    __hlt_gchdr __gch;
    binpac_parser* parser;
    struct __mime_parser* next;
} __mime_parser;

__HLT_RTTI_GC_TYPE(__mime_parser, HLT_TYPE_BINPAC_MIME_PARSER);

extern const hlt_type_info hlt_type_info_hlt_bytes;

void __mime_parser_dtor(hlt_type_info* ti, __mime_parser* mime_parser)
{
    GC_CLEAR(mime_parser->parser, hlt_Parser);
    GC_CLEAR(mime_parser->next, __mime_parser);
}

static void __add_parser(hlt_bytes* mt, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __mime_parser* mp = GC_NEW(__mime_parser);
    GC_INIT(mp->parser, parser, hlt_Parser);
    mp->next = 0;

    // Deep-copy the pointers, the map needs that.

    __mime_parser** cmp = hlt_malloc(sizeof(__mime_parser*));
    hlt_bytes**     cmt = hlt_malloc(sizeof(hlt_bytes*));
    *cmt = mt;
    *cmp = mp;

    __mime_parser** current = hlt_map_get_default(__binpac_globals_get()->mime_types, &hlt_type_info_hlt_bytes, cmt, &hlt_type_info___mime_parser, 0, excpt, ctx);

    if ( current )
        mp->next = *current; // +1 already.

    hlt_map_insert(__binpac_globals_get()->mime_types, &hlt_type_info_hlt_bytes, cmt, &hlt_type_info___mime_parser, cmp, excpt, ctx);

    GC_DTOR(mp, __mime_parser);

    hlt_free(cmp);
    hlt_free(cmt);

#ifdef DEBUG
    hlt_string ascii = hlt_string_from_asciiz("ascii", excpt, ctx);
    hlt_string s = hlt_string_decode(mt, ascii, excpt, ctx);

    char* r1 = hlt_string_to_native(s, excpt, ctx);
    char* r2 = hlt_string_to_native(parser->name, excpt, ctx);

    DBG_LOG("binpac-sinks", "MIME type %s registered for parser %s", r1, r2);

    GC_DTOR(ascii, hlt_string);
    GC_DTOR(s, hlt_string);
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
        hlt_iterator_bytes j = hlt_iterator_bytes_incr(i, excpt, ctx);
        result = hlt_bytes_sub(begin, j, excpt, ctx);
        GC_DTOR(j, hlt_iterator_bytes);
    }

    GC_DTOR(i, hlt_iterator_bytes);
    GC_DTOR(begin, hlt_iterator_bytes);
    GC_DTOR(end, hlt_iterator_bytes);

    return result;
}

static void __connect_one(binpac_sink* sink, hlt_bytes* mtype, hlt_bytes* mtype_full, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! __binpac_globals_get()->mime_types )
        return;

    __mime_parser** current = hlt_map_get_default(__binpac_globals_get()->mime_types, &hlt_type_info_hlt_bytes, &mtype, &hlt_type_info___mime_parser, 0, excpt, ctx);

    if ( ! current )
        return;

    __mime_parser* mparser = *current;
    for ( __mime_parser* mp = mparser; mp; mp = mp->next ) {
        assert(mp->parser->new_func);

        // The C stub for the ctor function may mess with our yield/resume
        // information. We need to restore that afterwards.
        hlt_fiber* saved_fiber = ctx->fiber;
        __hlt_thread_mgr_blockable* saved_blockable = ctx->blockable;

        void* pobj = mp->parser->new_func(sink, mtype, cookie, excpt, ctx);
        assert(pobj);

        ctx->fiber = saved_fiber;
        ctx->blockable = saved_blockable;

        _binpachilti_sink_connect_intern(sink, mp->parser->type_info, &pobj, mp->parser, mtype_full, excpt, ctx);

        GC_DTOR_GENERIC(&pobj, mp->parser->type_info);
    }

    GC_DTOR(mparser, __mime_parser);
}

void binpachilti_mime_register_parser(binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! (parser->mime_types && parser->new_func) )
         return;

    hlt_string ascii = hlt_string_from_asciiz("ascii", excpt, ctx);

    hlt_iterator_list i = hlt_list_begin(parser->mime_types, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parser->mime_types, excpt, ctx);

    while ( ! (hlt_iterator_list_eq(i, end, excpt, ctx) || *excpt) ) {
        hlt_string mt = *(hlt_string*) hlt_iterator_list_deref(i, excpt, ctx);
        hlt_bytes* b = hlt_string_encode(mt, ascii, excpt, ctx);
        GC_DTOR(mt, hlt_string);

        hlt_bytes_size len = hlt_bytes_len(b, excpt, ctx);
        int8_t* raw = hlt_bytes_to_raw(b, excpt, ctx);

        // If it ends in '/*', register with just the main type.
        if ( len > 2 && raw[len-1] == '*' && raw[len-2] == '/' ) {
            hlt_bytes* main = __main_type(b, excpt, ctx);
            assert(main);
            GC_DTOR(b, hlt_bytes);
            b = main;
        }

        // If it's the catch-all "*", add an empty string.
        if ( len == 1 && raw[0] == '*' ) {
            GC_DTOR(b, hlt_bytes);
            b = hlt_bytes_new(excpt, ctx);
        }

        hlt_free(raw);

        __add_parser(b, parser, excpt, ctx);
        GC_DTOR(b, hlt_bytes);

        hlt_iterator_list j = i;
        i = hlt_iterator_list_incr(i, excpt, ctx);
        GC_DTOR(j, hlt_iterator_list);
    }

    GC_DTOR(i, hlt_iterator_list);
    GC_DTOR(end, hlt_iterator_list);
    GC_DTOR(ascii, hlt_string);
}

void binpachilti_sink_connect_mimetype_string(binpac_sink* sink, hlt_string mtype, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string ascii = hlt_string_from_asciiz("ascii", excpt, ctx);
    hlt_bytes* b = hlt_string_encode(mtype, ascii, excpt, ctx);

    binpachilti_sink_connect_mimetype_bytes(sink, b, cookie, excpt, ctx);

    GC_DTOR(ascii, hlt_string);
    GC_DTOR(b, hlt_bytes);
}

void binpachilti_sink_connect_mimetype_bytes(binpac_sink* sink, hlt_bytes* mtype, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __connect_one(sink, mtype, mtype, cookie, excpt, ctx);

    // Do a second check just for the main type.
    hlt_bytes* mtype2 = __main_type(mtype, excpt, ctx);
    if ( mtype2 ) {
        __connect_one(sink, mtype2, mtype, cookie, excpt, ctx);
        GC_DTOR(mtype2, hlt_bytes);
    }

    // Check for catch-alls with the empty string.
    hlt_bytes* mtype3 = hlt_bytes_new(excpt, ctx);
    __connect_one(sink, mtype3, mtype, cookie, excpt, ctx);
    GC_DTOR(mtype3, hlt_bytes);
}
