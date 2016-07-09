
#ifndef HILTI_CODEGEN_OPTIMIZER_H
#define HILTI_CODEGEN_OPTIMIZER_H

#include "common.h"

namespace hilti {

class CompilerContext;
class Options;

namespace codegen {

/// HILTI's optimizer. Currently, it applies only LLVM-level optimizations
/// but eventually it will optimize HILTI programs as well.
class Optimizer : public ast::Logger {
public:
    /// Constructor.
    ///
    /// ctx: The compiler context to use.
    Optimizer(CompilerContext* ctx);

    /// Returns the compiler context the code generator is used with.
    CompilerContext* context() const;

    /// Returns the options in effecty for code generation. This is a
    /// convienience method that just forwards to the current context.
    const Options& options() const;

    /// Optimizes an LLVM module according to the CompilerContext's options.
    ///
    /// module: The module to optimize.
    ///
    /// is_linked: True if this is the final linked module, false if it's
    /// an individual module that will later be linked.
    ///
    /// Returns: A new, optmized module, or null on error.
    std::unique_ptr<llvm::Module> optimize(std::unique_ptr<llvm::Module> module, bool is_linked);

private:
    CompilerContext* _ctx;
};
}
}

#endif
