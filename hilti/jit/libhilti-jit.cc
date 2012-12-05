
#include "libhilti-jit.h"
#include "context.h"

struct __hlt_global_state;
extern "C" { void __hlt_init_from_state(struct __hlt_global_state* state); }

void hlt_init_jit(std::shared_ptr<hilti::CompilerContext> ctx)
{
    auto f = ctx->nativeFunction("hlt_init");
    auto init = (void (*)())f;
    assert(init);

    f = ctx->nativeFunction("__hlt_globals");
    auto func_globals = (__hlt_global_state* (*)())f;
    assert(func_globals);

    (*init)();
    __hlt_init_from_state(((*func_globals)()));

    f = ctx->nativeFunction("binpac_init");
    auto binpac_init = (void (*)())f;

    if ( binpac_init )
        (*binpac_init)();
}

