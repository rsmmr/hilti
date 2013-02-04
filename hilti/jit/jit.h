
#ifndef HILTI_JIT_JIT_H
#define HILTI_JIT_JIT_H

#include <ast/logger.h>

#include "../context.h"

namespace llvm {
    class ExecutionEngine;
    class SectionMemoryManager;
}

namespace hilti {

namespace jit {

class MemoryManager;

// Central JIT engine.
class JIT : public ast::Logger
{
public:
    typedef CompilerContext::FunctionMapping FunctionMapping;

    JIT(CompilerContext* ctx);
    ~JIT();

    /// JITs an LLVM module retuned by linkModules().
    ///
    /// module: The module. The function takes ownership.
    ///
    /// Returns: The execution engine to use with nativeFunction(). Null on
    /// error; an error message will have been reported.
    llvm::ExecutionEngine* jitModule(llvm::Module* module);

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

    /// Installs a table to resolve functions that are located statically
    /// inside the main process.
    ///
    /// mappings: An array of name-to-address mappings. The last entry must
    /// be null pointers to mark the end of the array.
    void installFunctionTable(const FunctionMapping* ftable);

private:
    CompilerContext* _ctx;
    MemoryManager* _mm;
};

}

}

#endif
