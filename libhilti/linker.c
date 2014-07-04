
#include <stdio.h>
#include <stdlib.h>

#include <autogen/cmake-config.h>

#include "linker.h"
#include "stackmap.h"

static struct __hlt_linker_functions* _functions = 0;

__attribute__ ((weak)) void __hlt_modules_init(void* ctx)
{
    if ( _functions && _functions->__hlt_modules_init )
        (*_functions->__hlt_modules_init)(ctx);
}

__attribute__ ((weak)) void __hlt_globals_init(void* ctx)
{
    if ( _functions && _functions->__hlt_globals_init )
        (*_functions->__hlt_globals_init)(ctx);
}

__attribute__ ((weak)) void __hlt_globals_dtor(void* ctx)
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

#ifdef HAVE_LLVM_STACKMAPS

// This is ugly. LLVM puts the stackmap into a section called
// ".llvm_stackmaps", which we can't access directly from C because of the
// leading dot. We hence add our own linker script that maps the start of the
// section to a custom identifier. We link it weak so that it's fine to link
// without the linker script in cases where we don't actually initialize the
// runtime.
#ifdef __linux__
extern __llvm_stackmap __start_LLVM_STACKMAPS __attribute__ ((weak));
#else
#error No stackmap support on this platform.
#endif

#endif

__llvm_stackmap* __hlt_llvm_stackmap()
{
#ifdef HAVE_LLVM_STACKMAPS
    return _functions && _functions->__stackmap ? _functions->__stackmap : &__start_LLVM_STACKMAPS;
#else
    fprintf(stderr, "no stackmap support compiled in\n");
    abort();
#endif
}

void __hlt_linker_set_functions(struct __hlt_linker_functions* funcs)
{
    _functions = funcs;
    // fprintf(stderr, "X3 %p %p %p\n", _functions->__hlt_globals_dtor, &_functions, _functions);
}
