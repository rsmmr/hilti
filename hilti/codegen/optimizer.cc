
#include "optimizer.h"
#include "../context.h"
#include "../options.h"
#include "codegen.h"
#include "llvm-common.h"
#include "util.h"

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

std::unique_ptr<llvm::Module> Optimizer::optimize(std::unique_ptr<llvm::Module> module,
                                                  bool is_linked)
{
#if 0
    std::error_code lerr;
    llvm::raw_fd_ostream out(std::string("optmodule.bc"), lerr, llvm::sys::fs::F_None);
    llvm::WriteBitcodeToFile(module.get(), out);
    out.close();
#endif

#if 0
    llvm::LLVMContext nctx;

    auto b = llvm::MemoryBuffer::getFile("optmodule.bc");
    auto m = llvm::parseBitcodeFile((*b)->getMemBufferRef(), module->getContext());
#endif

    // TODO: There's some problem with LLVM mixing up debug symbols between
    // the originating modules if we just go ahead here and optmize the
    // module that was passed in:
    //
    // Global is referenced in a different module!
    // void (metadata, metadata, metadata)* @llvm.dbg.declare
    // ; ModuleID = '<JIT code>'
    // call void @llvm.dbg.declare(metadata i8** %2, metadata <0x5506300>, metadata !6081), !dbg
    // <0x5501ee8>
    // void (i8*)* @__hlt_globals_dtor
    //
    // [plus a couple more of these]
    //
    // To work around that until understood/fixed, we clone the module
    // through serialization, which then doesn't show the problem anymore.
    llvm::SmallVector<char, 128> buffer;
    llvm::raw_svector_ostream os(buffer);
    llvm::WriteBitcodeToFile(module.get(), os);
    auto mb = std::make_unique<llvm::ObjectMemoryBuffer>(std::move(buffer));
    auto nmodule = llvm::parseBitcodeFile((*mb).getMemBufferRef(), module->getContext());

#if 1
    // Logic borrowed heavily from opt.

    llvm::PassRegistry& registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(registry);
    llvm::initializeScalarOpts(registry);
    llvm::initializeObjCARCOpts(registry);
    llvm::initializeVectorization(registry);
    llvm::initializeIPO(registry);
    llvm::initializeAnalysis(registry);
    llvm::initializeTransformUtils(registry);
    llvm::initializeInstCombine(registry);
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
    llvm::initializeCodeGen(registry); // all codegen passes

    std::string err;

    int opt_level;
    llvm::CodeGenOpt::Level cg_opt_level;

    switch ( options().opt_level ) {
    case 1: // -O1
        opt_level = 1;
        cg_opt_level = llvm::CodeGenOpt::Less;
        break;

    case 2: // -O2
        opt_level = 2;
        cg_opt_level = llvm::CodeGenOpt::Default;
        break;

    case 3: // -O2
        opt_level = 3;
        cg_opt_level = llvm::CodeGenOpt::Aggressive;
        break;

    default:
        fatalError(
            ::util::fmt("optimizer: unsupported optimization level %d", options().opt_level));
    }

    // Auto-detect CPU features.
    llvm::SubtargetFeatures cpu_features;
    llvm::StringMap<bool> host_features;
    if ( llvm::sys::getHostCPUFeatures(host_features) ) {
        for ( auto& f : host_features )
            cpu_features.AddFeature(f.first(), f.second);
    }

    llvm::Triple triple((*nmodule)->getTargetTriple());
    llvm::TargetOptions topts; // Believe they are all fine with their defaults.
    auto target = llvm::TargetRegistry::lookupTarget(triple.getTriple(), err);

    if ( ! target )
        fatalError(::util::fmt("optimizer: cannot determine target, %s", err));

    auto tm_p = target->createTargetMachine(triple.getTriple(), llvm::sys::getHostCPUName(),
                                            cpu_features.getString(), topts, llvm::Reloc::PIC_,
                                            llvm::CodeModel::Default, cg_opt_level);

    if ( ! target )
        fatalError(::util::fmt("optimizer: cannot create taget machine"));

    std::unique_ptr<llvm::TargetMachine> tm(tm_p);

    llvm::PassManagerBuilder builder;
    builder.VerifyInput = true;
    builder.VerifyOutput = false;
    builder.OptLevel = opt_level;
    builder.SizeLevel = 0;
    builder.Inliner =
        (opt_level > 1 ? llvm::createFunctionInliningPass(opt_level, 0 /* SizeLevel */) :
                         llvm::createAlwaysInlinerPass());
    builder.LoopVectorize = (opt_level > 1);
    builder.SLPVectorize = (opt_level > 1);

    builder.addExtension(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                         [&](const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& pm) {
                             tm->addEarlyAsPossiblePasses(pm);
                         });

    std::unique_ptr<llvm::legacy::FunctionPassManager> fpasses;
    fpasses.reset(new llvm::legacy::FunctionPassManager(&**nmodule));
    fpasses->add(llvm::createTargetTransformInfoWrapperPass(tm->getTargetIRAnalysis()));

    llvm::legacy::PassManager passes;
    passes.add(llvm::createTargetTransformInfoWrapperPass(tm->getTargetIRAnalysis()));
    passes.add(llvm::createVerifierPass());

    builder.populateFunctionPassManager(*fpasses);
    builder.populateLTOPassManager(passes);
    builder.populateModulePassManager(passes);

    // Run function passes.
    fpasses->doInitialization();

    for ( llvm::Function& f : *nmodule.get() )
        fpasses->run(f);

    fpasses->doFinalization();

#if 0
    llvm::AssemblyAnnotationWriter annotator;
    llvm::raw_os_ostream llvm_out(std::cerr);
    nmodule->print(llvm_out, &annotator);
#endif

    // Check that the module is well-formed on completion of optimization.
    passes.add(llvm::createVerifierPass());

    // Run module passes.
    if ( passes.run(**nmodule) )
        return std::move(nmodule.get());
    else
        return nullptr;
#endif
}
