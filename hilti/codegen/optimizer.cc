
#include "optimizer.h"
#include "util.h"
#include "codegen.h"
#include "../options.h"
#include "../context.h"

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
#if 0
    string lerr;
    llvm::raw_fd_ostream out("optmodule.bc", lerr);
    llvm::WriteBitcodeToFile(module, out);
    out.close();
#endif

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
#ifdef HAVE_LLVM_33
    to.JITExceptionHandling = false;
#endif
    to.JITEmitDebugInfo = false;
    to.JITEmitDebugInfoToDisk = false;

    auto tm = target->createTargetMachine(triple, llvm::sys::getHostCPUName(), "" /* CPU features */, to, llvm::Reloc::Default, llvm::CodeModel::Default, llvm::CodeGenOpt::Default);

    tm->addAnalysisPasses(passes);

    llvm::PassManagerBuilder builder;
    builder.Inliner = llvm::createFunctionInliningPass();
    // builder.Inliner = llvm::createAlwaysInlinerPass();
    builder.LibraryInfo = new llvm::TargetLibraryInfo(llvm::Triple(module->getTargetTriple()));
    builder.OptLevel = 2;
    builder.SizeLevel = 0;
    builder.DisableUnitAtATime = false;
    builder.DisableUnrollLoops = false;
#ifdef HAVE_LLVM_33
    builder.DisableSimplifyLibCalls = false;
#else
    builder.LoopVectorize = (builder.OptLevel > 1 && builder.SizeLevel < 2);
    builder.SLPVectorize = true;
#endif

    // *If* we optimized after compiling modules, we could maybe skip the
    // following two lines for the linked module. However, we do not (see
    // comment in CompilerContext::Compile). Doing so would however probably
    // speed up the link time quite a bit.
    // if ( ! is_linked ) {
    builder.populateFunctionPassManager(fpasses);
    builder.populateModulePassManager(passes);
    // }

    if ( is_linked ) {
        // Internalize doesn't work unfortunately because we interface with
        // the host application.
        //
        // Also, if activated in JIT mode, the LLVM IPSCCP pass crashes in
        // lib/Transforms/Scalar/SCCP.cpp, line 1970 (StoreInst *SI =
        // cast<StoreInst>(GV->use_back());) when workin on
        // @_functions = internal unnamed_addr global %struct.__hlt_linker_functions* null, align 8:
        //
        // bro: /opt/llvm-git/src/llvm/include/llvm/Support/Casting.h:239:
        // typename cast_retty<X, Y *>::ret_type llvm::cast(Y *) [X =
        // llvm::StoreInst, Y = llvm::User]: Assertion `isa<X>(Val) &&
        // "cast<Ty>() argument of incompatible type!"' failed.
        bool internalize = false;
        builder.populateLTOPassManager(passes, internalize, true);
    }

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
