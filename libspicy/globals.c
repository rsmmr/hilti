

#include "globals.h"
#include "libspicy.h"
#include "mime.h"

static __spicy_global_state __our_globals;
static __spicy_global_state* __globals = &__our_globals;

void* __spicy_runtime_state_get()
{
    return __globals;
}

void __spicy_runtime_state_set(void* state)
{
    __globals = (__spicy_global_state*)state;
}

extern const hlt_type_info hlt_type_info___mime_parser;

int __spicy_global_state_init()
{
    if ( __globals->initialized )
        return 0;

    __globals->initialized = 1;

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    __globals->parsers = hlt_list_new(&hlt_type_info_hlt_SpicyHilti_Parser, 0, &excpt, ctx);
    GC_CCTOR(__globals->parsers, hlt_list, ctx);

    __globals->mime_types =
        hlt_map_new(&hlt_type_info_hlt_bytes, &hlt_type_info___mime_parser, 0, &excpt, ctx);
    GC_CCTOR(__globals->mime_types, hlt_map, ctx);

    __globals->debugging = 0;

    return 1;
}

int __spicy_global_state_done()
{
    if ( ! __globals->initialized )
        return 0;

    if ( __globals->finished )
        return 0;

    GC_DTOR(__globals->parsers, hlt_list, hlt_global_execution_context());
    GC_DTOR(__globals->mime_types, hlt_map, hlt_global_execution_context());

    __globals->finished = 1;

    return 1;
}

int __spicy_globals_initialized()
{
    return (__globals->initialized && ! __globals->finished);
}

__spicy_global_state* __spicy_globals()
{
#ifdef DEBUG
    if ( __globals->finished ) {
        fprintf(stderr, "internal error: libspicy globals accessed after done\n");
        abort();
    }

    if ( ! __globals->initialized ) {
        fprintf(stderr, "internal error: libspicy globals accessed before intialization\n");
        abort();
    }
#endif

    assert(__globals->initialized);
    return __globals;
}
