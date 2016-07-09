
#ifndef HILTI_JIT_JIT_H
#define HILTI_JIT_JIT_H

#include <ast/logger.h>

#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

namespace hilti {

class CompilerContext;

// Central JIT engine.
class JIT : public ast::Logger {
public:
    /// Constructor that JITs an LLVM module retuned by linkModules().
    ///
    /// ctx: The HILTI compiler context.
    JIT(CompilerContext* ctx);
    ~JIT();

    /// JITs an LLVM module retuned by linkModules(), and then initializes
    /// the runtime library. Afterwards nativeFunction() can be used to
    /// retrieve the address of compiled functions.
    ///
    /// To set runtime options, the host application can use
    /// hlt_config_get/set() before calling this method.
    ///
    /// module: The module. The function takes ownership.
    ///
    /// Returns: True if JITing succeeded. If *main_run* is true, it will
    /// have finished by the time the function returns.
    bool jit(std::unique_ptr<llvm::Module> module);

    /// Returns a pointer to a compiled, native function after a module has
    /// beed JITed. Must only be called after jit() signaled success.
    ///
    /// function: The name of the function.
    ///
    /// must_exist: If true, the method aborts the process with a fatal error
    /// if the function does not exist in the compiled code.
    ///
    /// Returns: A pointer to the function (which must be suitably casted),
    /// or null if that function doesn't exist and *must_exist* is false.
    void* nativeFunction(const string& function, bool must_exist = true);

private:
    CompilerContext* _ctx;

    typedef llvm::orc::ObjectLinkingLayer<> ObjectLayer;
    typedef llvm::orc::IRCompileLayer<ObjectLayer> CompileLayer;

    std::unique_ptr<llvm::DataLayout> _data_layout;
    std::unique_ptr<llvm::TargetMachine> _target_machine;
    std::unique_ptr<ObjectLayer> _object_layer;
    std::unique_ptr<CompileLayer> _compile_layer;
};
}

#endif
