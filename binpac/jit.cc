
#include "jit.h"

extern "C" {
extern void* __binpac_runtime_state_get();
extern void __binpac_runtime_state_set(void* state);
}

using namespace binpac;

JIT::JIT(hilti::CompilerContext* ctx) : hilti::JIT(ctx)
{
}

JIT::~JIT()
{
    auto jit_binpac_done = (void (*)())nativeFunction("__binpac_done");
    (*jit_binpac_done)();
}

void JIT::_jit_init()
{
    // Same scheme here as in hilti::JIT::jit().
    auto hrss = (void (*)(void*))nativeFunction("__binpac_runtime_state_set");

    (*hrss)(__binpac_runtime_state_get());

    auto jit_binpac_init = (void (*)())nativeFunction("__binpac_init");
    (*jit_binpac_init)();
}
