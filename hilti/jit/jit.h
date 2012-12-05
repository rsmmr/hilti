
#ifndef HILTI_JIT_JIT_H
#define HILTI_JIT_JIT_H

#include <ast/logger.h>

namespace llvm {
    class ExecutionEngine;
    class SectionMemoryManager;
}

namespace hilti {

class CompilerContext;

namespace jit {

// Central JIT engine.
class JIT : public ast::Logger
{
public:
    JIT(CompilerContext* ctx);
    ~JIT();

    /// JITs an LLVM module retuned by linkModules().
    ///
    /// module: The module. The function takes ownership.
    ///
    /// optimize: The optimization level, corresponding to \c -Ox
    ///
    /// Returns: The execution engine to use with nativeFunction(). Null on
    /// error; an error message will have been reported.
    llvm::ExecutionEngine* jitModule(llvm::Module* module, int optimize);

    /// Returns a pointer to a compiled, native function after a module has beed JITed.
    ///
    /// ee: The engine returned by jitModule().
    ///
    /// module: The module that defines the target function.
    ///
    /// function: The name of the function.
    ///
    /// Returns: A pointer to the function (which must be suitably casted),
    /// or null if that function doesn't exist.
    void* nativeFunction(llvm::ExecutionEngine* ee, llvm::Module* module, const string& function);

private:
    CompilerContext* _ctx;
    llvm::SectionMemoryManager* _mm;
};

}

}

#endif
