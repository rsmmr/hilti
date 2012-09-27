//
// BinPAC++ functionaly supporting the generated parsers.
//

#include <stdio.h>
#include <stdlib.h>

#include "libbinpac++.h"

static int       _initialized = 0;
static int       _done = 0;
static hlt_list* _parsers = 0;
static int8_t    _debugging = 0;

static void _ensure_parsers(hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! _parsers )
        _parsers = hlt_list_new(hlt_type_info_hlt_Parser, 0, excpt, ctx);
}

void binpac_init()
{
    _initialized = 1;
    atexit(binpac_done);
}

void binpac_done()
{
    if ( _done )
        return;

    _done = 1;

    GC_DTOR(_parsers, hlt_list);
}

hlt_list* binpac_parsers(hlt_exception** excpt, hlt_execution_context* ctx)
{
    _ensure_parsers(excpt, ctx);
    GC_CCTOR(_parsers, hlt_list);
    return _parsers;
}

void binpac_enable_debugging(int8_t enabled)
{
    // Should be (sufficiently) thread-safe.
    _debugging = enabled;
}

int8_t binpac_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return _debugging;
}

void binpac_fatal_error(const char* msg)
{
    fprintf(stderr, "fatal binpac error: %s", msg);
}

// Note that this function can be called before binpac_init().
void binpachilti_register_parser(binpac_parser* parser, hlt_type_info* pobj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    _ensure_parsers(excpt, ctx);
    parser->type_info = pobj;
    hlt_list_push_back(_parsers, hlt_type_info_hlt_Parser, &parser, excpt, ctx);

#if 0
    binpac_mime_register_parser(parser, excpt, ctx);
#endif
}

void call_init_func(void (*func)(hlt_exception** excpt, hlt_execution_context* ctx))
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    (*func)(&excpt, ctx);

    if ( excpt )
        hlt_exception_print_uncaught(excpt, ctx);
}

void binpac_debug_print_ptr(hlt_string tag, const hlt_type_info* type, void** ptr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* s = hlt_string_to_native(tag, excpt, ctx);
    fprintf(stderr, "debug: %s %p\n", s, *ptr);
}
