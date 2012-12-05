
#include <llvm/ExecutionEngine/MCJIT.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/Host.h>
#include <llvm/Module.h>

#include "jit.h"
#include "llvm/SectionMemoryManager.h"

using namespace hilti;
using namespace hilti::jit;

JIT::JIT(CompilerContext* ctx)
{
    _ctx = ctx;
    _mm = new llvm::SectionMemoryManager;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
}

JIT::~JIT()
{
}

llvm::ExecutionEngine* JIT::jitModule(llvm::Module* module, int optimize)
{
    auto opt = llvm::CodeGenOpt::Default;

    switch ( optimize ) {
     case 0:
        opt = llvm::CodeGenOpt::None;
        break;

     case 1:
        opt = llvm::CodeGenOpt::Less;
        break;

     case 2:
        opt = llvm::CodeGenOpt::Default;
        break;

     case 3:
        opt = llvm::CodeGenOpt::Aggressive;
        break;

     default:
        error("unsupported optimization level");
        return 0;
    }

    string errormsg;

    llvm::EngineBuilder builder(module);
    builder.setEngineKind(llvm::EngineKind::JIT);
    builder.setUseMCJIT(true);
    builder.setJITMemoryManager(_mm);
    builder.setErrorStr(&errormsg);
    builder.setOptLevel(opt);
    builder.setAllocateGVsWithCode(false);
    builder.setCodeModel(llvm::CodeModel::JITDefault);
    builder.setRelocationModel(llvm::Reloc::Default);
    builder.setMArch("");
    builder.setMCPU(llvm::sys::getHostCPUName());

    llvm::TargetOptions Options;
    Options.JITExceptionHandling = false;
    Options.JITEmitDebugInfo = true;
    Options.JITEmitDebugInfoToDisk = true;
    builder.setTargetOptions(Options);

    auto ee = builder.create();
    if ( ! ee ) {
        error(util::fmt("LLVM jit error: %s", errormsg));
        return nullptr;
    }

    // ee->DisableLazyCompilation(true);
    // ee->runStaticConstructorsDestructors(false);

    return ee;
}

void* JIT::nativeFunction(llvm::ExecutionEngine* ee, llvm::Module* module, const string& function)
{
    auto func = module->getFunction(function);

    if ( ! func )
        return 0;

    auto fp = ee->getPointerToFunction(func);

    if ( ! fp ) {
        error(util::fmt("jit: cannt get pointer to function %s in module %s", function, module->getModuleIdentifier()));
        return 0;
    }

    _mm->invalidateInstructionCache();
    return fp;
}
