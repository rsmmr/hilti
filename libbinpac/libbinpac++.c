//
// BinPAC++ functionaly supporting the generated parsers.
//

#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "libbinpac++.h"
#include "mime.h"

struct _tmp_parser_def {
    binpac_parser* parser;
    hlt_type_info* pobj;
};

static struct _tmp_parser_def* _tmp_parsers = 0;
static int _tmp_parsers_size = 0;

static void _register_parser(binpac_parser* parser, hlt_type_info* pobj, hlt_exception** excpt,
                             hlt_execution_context* ctx)
{
    parser->type_info = pobj;
    hlt_list_push_back(__binpac_globals()->parsers, &hlt_type_info_hlt_BinPACHilti_Parser, &parser,
                       excpt, ctx);
    binpachilti_mime_register_parser(parser, excpt, ctx);
    GC_DTOR(parser, hlt_BinPACHilti_Parser, ctx);
}

void __binpac_register_pending_parsers()
{
    if ( ! _tmp_parsers )
        return;

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    for ( int i = 0; i < _tmp_parsers_size; i++ )
        _register_parser(_tmp_parsers[i].parser, _tmp_parsers[i].pobj, &excpt, ctx);

    free(_tmp_parsers);
}

hlt_list* binpac_parsers(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return __binpac_globals()->parsers;
}

void binpac_enable_debugging(int8_t enabled)
{
    // Should be (sufficiently) thread-safe.
    __binpac_globals()->debugging = enabled;
}

int8_t binpac_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return __binpac_globals()->debugging;
}

int8_t binpachilti_debugging_enabled(hlt_exception** excpt, hlt_execution_context* ctx)
{
    return binpac_debugging_enabled(excpt, ctx);
}

void binpac_fatal_error(const char* msg)
{
    fprintf(stderr, "fatal binpac error: %s", msg);
}

// Note that this function can be called before binpac_init().
void binpachilti_register_parser(binpac_parser* parser, hlt_type_info* pobj, hlt_exception** excpt,
                                 hlt_execution_context* ctx)
{
    GC_CCTOR(parser, hlt_BinPACHilti_Parser, ctx);

    if ( __binpac_globals_initialized() )
        _register_parser(parser, pobj, excpt, ctx);

    else {
        int new_size = (_tmp_parsers_size + 1);
        _tmp_parsers = realloc(_tmp_parsers, new_size * sizeof(struct _tmp_parser_def));
        _tmp_parsers[_tmp_parsers_size].parser = parser;
        _tmp_parsers[_tmp_parsers_size].pobj = pobj;
        _tmp_parsers_size = new_size;
    }
}

void call_init_func(void (*func)(hlt_exception** excpt, hlt_execution_context* ctx))
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    (*func)(&excpt, ctx);

    if ( excpt )
        hlt_exception_print_uncaught(excpt, ctx);
}

void binpac_debug_print_ptr(hlt_string tag, const hlt_type_info* type, void** ptr,
                            hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(false);
}
