
#include "libhilti-jit.h"
#include "context.h"

extern "C" {
    #include "libhilti/linker.h"

    struct __hlt_global_state;
    struct __binpac_globals;
    struct hlt_config;

    void __hlt_init_from_state(struct __hlt_global_state* state);
    void __binpac_init_from_state(struct __binpac_globals* state);

    __hlt_global_state*  __hlt_globals();
    __hlt_global_state*  __hlt_globals_object_no_init();

    __attribute__ ((weak)) __binpac_globals*  __binpac_globals_get() { return 0; }

    const hlt_config* hlt_config_get();

    void hlt_init();
}

// TODO: Maybe we don't need to do all this globals magic with redirecting the pointer via init_from_state.
//
// From: http://llvm.org/docs/tutorial/LangImpl4.html
//
//      The LLVM JIT provides a number of interfaces (look in the
//      ExecutionEngine.h file) for controlling how unknown functions get
//      resolved. It allows you to establish explicit mappings between IR
//      objects and addresses (useful for LLVM global variables that you want
//      to map to static tables, for example), allows you to dynamically
//      decide on the fly based on the function name, and even allows you to
//      have the JIT compile functions lazily the first time they’re called.
//
//
// ExecutionEngine::addGlobalMapping might be the right one:
//
//   http://llvm.org/docs/doxygen/html/classllvm_1_1ExecutionEngine.html#a805704b52a327cc6b37aebf9cba14169
//
// Update: Tried it, but doesn't seem to work.

static struct __hlt_linker_functions _funcs;

void hlt_init_jit(std::shared_ptr<hilti::CompilerContext> ctx, llvm::Module* module, llvm::ExecutionEngine* ee)
{
    auto f = ctx->nativeFunction(module, ee, "__hlt_init_from_state");
    auto hlt_init_from_state = (void (*)(__hlt_global_state*))f;
    assert(hlt_init_from_state);

    f = ctx->nativeFunction(module, ee, "hlt_config_set");
    auto hlt_config_set = (void (*)(const hlt_config*))f;
    assert(hlt_config_set);

    f = ctx->nativeFunction(module, ee, "__hlt_modules_init");
    auto modules_init = (void (*)(__hlt_execution_context*))f;

    f = ctx->nativeFunction(module, ee, "__hlt_globals_init");
    auto globals_init = (void (*)(__hlt_execution_context*))f;

    f = ctx->nativeFunction(module, ee, "__hlt_globals_dtor");
    auto globals_dtor = (void (*)(__hlt_execution_context*))f;

    f = ctx->nativeFunction(module, ee, "__hlt_globals_size");
    auto globals_size = (int64_t (*)())f;

    _funcs.__hlt_modules_init = modules_init;
    _funcs.__hlt_globals_init = globals_init;
    _funcs.__hlt_globals_dtor = globals_dtor;
    _funcs.__hlt_globals_size = globals_size;
    __hlt_linker_set_functions(&_funcs);

    (*hlt_init_from_state)(__hlt_globals_object_no_init());
    hlt_init();
}

void binpac_init_jit(std::shared_ptr<hilti::CompilerContext> ctx, llvm::Module* module, llvm::ExecutionEngine* ee)
{
    auto f = ctx->nativeFunction(module, ee, "__binpac_init_from_state");
    auto binpac_init_from_state = (void (*)(__binpac_globals*))f;

    if ( binpac_init_from_state && __binpac_globals_get() )
        (*binpac_init_from_state)(__binpac_globals_get());
}

