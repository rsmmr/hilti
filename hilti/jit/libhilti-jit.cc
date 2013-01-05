
#include "libhilti-jit.h"
#include "context.h"

struct __hlt_global_state;
extern "C" { void __hlt_init_from_state(struct __hlt_global_state* state); }


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

void hlt_init_jit(std::shared_ptr<hilti::CompilerContext> ctx, llvm::Module* module, llvm::ExecutionEngine* ee)
{
    auto f = ctx->nativeFunction(module, ee, "hlt_init");
    auto init = (void (*)())f;
    assert(init);

    f = ctx->nativeFunction(module, ee, "__hlt_globals");
    auto func_globals = (__hlt_global_state* (*)())f;
    assert(func_globals);

    (*init)();
    __hlt_init_from_state(((*func_globals)()));

    f = ctx->nativeFunction(module, ee, "binpac_init");
    auto binpac_init = (void (*)())f;

    if ( binpac_init )
        (*binpac_init)();
}

