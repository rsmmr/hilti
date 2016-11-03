
#include "jit.h"

extern "C" {
extern void* __spicy_runtime_state_get();
extern void __spicy_runtime_state_set(void* state);
}

using namespace spicy;

JIT::JIT(hilti::CompilerContext* ctx) : hilti::JIT(ctx)
{
}

JIT::~JIT()
{
    auto jit_spicy_done = (void (*)())nativeFunction("__spicy_done");
    (*jit_spicy_done)();
}

void JIT::_jit_init()
{
    // Same scheme here as in hilti::JIT::jit().
    auto hrss = (void (*)(void*))nativeFunction("__spicy_runtime_state_set");

    (*hrss)(__spicy_runtime_state_get());

    auto jit_spicy_init = (void (*)())nativeFunction("__spicy_init");
    (*jit_spicy_init)();
}
