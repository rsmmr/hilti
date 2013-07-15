
#include "optimizer.h"
#include "util.h"
#include "codegen.h"
#include "options.h"
#include "context.h"

using namespace hilti;
using namespace codegen;

Optimizer::Optimizer(CompilerContext* ctx) : ast::Logger("codegen::Optimizer")
{
    llvm::InitializeNativeTarget();
    _ctx = ctx;
}

CompilerContext* Optimizer::context() const
{
    return _ctx;
}

const Options& Optimizer::options() const
{
    return _ctx->options();
}

bool Optimizer::optimize(llvm::Module* module, bool is_linked)
{
    // Logic borrowed loosely from LLVM's opt.

    llvm::PassManager passes;
    llvm::FunctionPassManager fpasses(module);

    passes.add(new llvm::DataLayout(module));
    fpasses.add(new llvm::DataLayout(module));

    auto triple = module->getTargetTriple();
    assert(triple.size());

    std::string err;
    auto target = llvm::TargetRegistry::lookupTarget(triple, err);

    if ( ! target ) {
        error(::util::fmt("optimizer: cannot determine target, %s", err));
        return nullptr;
    }

    llvm::TargetOptions to; // Note, this has tons of options. We use the defaults mostly.
    to.JITExceptionHandling = false;
    to.JITEmitDebugInfo = true;
    to.JITEmitDebugInfoToDisk = true;

    auto tm = target->createTargetMachine(triple, llvm::sys::getHostCPUName(), "" /* CPU features */, to, llvm::Reloc::Default, llvm::CodeModel::Default, llvm::CodeGenOpt::Aggressive);

    tm->addAnalysisPasses(passes);

    llvm::PassManagerBuilder builder;
    builder.Inliner = llvm::createFunctionInliningPass(255);
    builder.LibraryInfo = new llvm::TargetLibraryInfo(llvm::Triple(module->getTargetTriple()));
    builder.OptLevel = 3;
    builder.SizeLevel = 0;
    builder.DisableUnitAtATime = false;
    builder.DisableUnrollLoops = false;
    builder.DisableSimplifyLibCalls = false;

    builder.populateFunctionPassManager(fpasses);
    builder.populateModulePassManager(passes);

    if ( is_linked )
        // TODO: Can we internalize the functions? Maybe we can add a whitelist?
        builder.populateLTOPassManager(passes, false /* internalize */, true);

    // Check that the module is well formed on completion of optimization.
    passes.add(llvm::createVerifierPass());

    // Run function passes.
    fpasses.doInitialization();

    for ( auto f = module->begin(); f != module->end(); f++ )
        fpasses.run(*f);

    fpasses.doFinalization();

    // Run module passes.
    return passes.run(*module);
}
