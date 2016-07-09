
#include "globals.h"

extern int _initialized;

static __binpac_globals _our_globals;
__binpac_globals* _globals = 0;

__binpac_globals* __binpac_globals_get()
{
    assert(_globals);
    return _globals;
}

extern const hlt_type_info hlt_type_info___mime_parser;

void __binpac_globals_init()
{
    assert(! _globals);

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    _globals = &_our_globals;

    _globals->parsers = hlt_list_new(&hlt_type_info_hlt_BinPACHilti_Parser, 0, &excpt, ctx);
    GC_CCTOR(_globals->parsers, hlt_list, ctx);

    _globals->mime_types =
        hlt_map_new(&hlt_type_info_hlt_bytes, &hlt_type_info___mime_parser, 0, &excpt, ctx);
    GC_CCTOR(_globals->mime_types, hlt_map, ctx);

    _globals->debugging = 0;
}

void __binpac_globals_done()
{
    assert(_globals);
    GC_DTOR(_globals->parsers, hlt_list, hlt_global_execution_context());
    GC_DTOR(_globals->mime_types, hlt_map, hlt_global_execution_context());
}

void __binpac_globals_set(__binpac_globals* state)
{
    assert(! _globals);
    _globals = state;
}
