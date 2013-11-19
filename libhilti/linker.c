
#include <stdio.h>

#include "linker.h"

static struct __hlt_linker_functions* _functions = 0;

__attribute__ ((weak)) void __hlt_modules_init(void* ctx)
{
    if ( _functions && _functions->__hlt_modules_init )
        (*_functions->__hlt_modules_init)(ctx);
}

__attribute__ ((weak)) void __hlt_globals_init(struct __hlt_execution_context* ctx)
{
    if ( _functions && _functions->__hlt_globals_init )
        (*_functions->__hlt_globals_init)(ctx);
}

__attribute__ ((weak)) void __hlt_globals_dtor(struct __hlt_execution_context* ctx)
{
    // fprintf(stderr, "X2 %p %p %p\n", _functions->__hlt_globals_dtor, &_functions, _functions);

    if ( _functions && _functions->__hlt_globals_dtor )
        (*_functions->__hlt_globals_dtor)(ctx);
}

__attribute__ ((weak)) int64_t __hlt_globals_size()
{
    // fprintf(stderr, "X4 %p %p %p\n", _functions->__hlt_globals_size, &_functions, _functions);
    return _functions  && _functions->__hlt_globals_size ? (*_functions->__hlt_globals_size)() : 0;
}

void __hlt_linker_set_functions(struct __hlt_linker_functions* funcs)
{
    _functions = funcs;
    // fprintf(stderr, "X3 %p %p %p\n", _functions->__hlt_globals_dtor, &_functions, _functions);
}
