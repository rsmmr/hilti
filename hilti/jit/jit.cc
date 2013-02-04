
#include <llvm/ExecutionEngine/MCJIT.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/Host.h>
#include <llvm/Module.h>

#include "jit.h"
#include "options.h"
#include "llvm/SectionMemoryManager.h"

using namespace hilti;
using namespace hilti::jit;

class hilti::jit::MemoryManager : public llvm::SectionMemoryManager
{
public:
    void installFunctionTable(const JIT::FunctionMapping* functions);
    void* getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure = true) override;

private:
    const JIT::FunctionMapping* _functions = 0;
};

void MemoryManager::installFunctionTable(const JIT::FunctionMapping* functions)
{
    _functions = functions;
}

void* MemoryManager::getPointerToNamedFunction(const std::string& name, bool abort_on_failure)
{
    if ( _functions ) {
        for ( int i = 0; _functions[i].name; i++ ) {
            if ( name == _functions[i].name ) {
                void *addr = _functions[i].func;
                return addr;
            }
        }
    }

    return llvm::SectionMemoryManager::getPointerToNamedFunction(name, abort_on_failure);
}

JIT::JIT(CompilerContext* ctx)
{
    _ctx = ctx;
    _mm = new MemoryManager;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
}

JIT::~JIT()
{
}

llvm::ExecutionEngine* JIT::jitModule(llvm::Module* module)
{
    // Or llvm::CodeGenOpt::Aggressive? (-O3)
    auto opt = _ctx->options().optimize ? llvm::CodeGenOpt::Default : llvm::CodeGenOpt::None;

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

void JIT::installFunctionTable(const FunctionMapping* mappings)
{
    _mm->installFunctionTable(mappings);
}
