
#include <llvm/ExecutionEngine/MCJIT.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include "jit.h"
#include "../options.h"

extern void* __hlt_internal_global_globals;

using namespace hilti;
using namespace hilti::jit;

// This is a proxy class that forwards most method calls directly to LLVM's
// DefaultJITMemoryManager. This is necessary because that class is not
// exposed for reasons I don't understand.
class hilti::jit::MemoryManager : public llvm::JITMemoryManager
{
public:
    MemoryManager(llvm::JITMemoryManager *mm);

    void installFunctionTable(const JIT::FunctionMapping* functions);

    // This is one method we override with our own version.
    void* getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure = true) override;

    // Proxy methods.
    void AllocateGOT() override { _mm->AllocateGOT(); }
    size_t GetDefaultCodeSlabSize() override { return _mm->GetDefaultCodeSlabSize(); }
    size_t GetDefaultDataSlabSize() override { return _mm->GetDefaultDataSlabSize(); }
    size_t GetDefaultStubSlabSize() override { return _mm->GetDefaultStubSlabSize(); }
    unsigned GetNumCodeSlabs() override { return _mm->GetNumCodeSlabs(); }
    unsigned GetNumDataSlabs() override { return _mm->GetNumDataSlabs(); }
    unsigned GetNumStubSlabs() override { return _mm->GetNumStubSlabs(); }
    uint8_t *allocateSpace(intptr_t Size, unsigned Alignment) override { return _mm->allocateSpace(Size, Alignment); }
    uint8_t *allocateStub(const llvm::GlobalValue* F, unsigned StubSize, unsigned Alignment) override { return _mm->allocateStub(F, StubSize, Alignment); }
    uint8_t *allocateGlobal(uintptr_t Size, unsigned Alignment) override { return _mm->allocateGlobal(Size, Alignment); }
    uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID) override { return _mm->allocateCodeSection(Size, Alignment, SectionID); }
    uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, bool IsReadOnly) override { return _mm->allocateDataSection(Size, Alignment, SectionID, IsReadOnly); }
    bool applyPermissions(std::string *ErrMsg) override { return _mm->applyPermissions(ErrMsg); }
    uint8_t *getGOTBase() const { return _mm->getGOTBase(); }
    void deallocateFunctionBody(void *Body) override { _mm->deallocateFunctionBody(Body); }
    void setMemoryWritable() override { _mm->setMemoryWritable(); }
    void setMemoryExecutable() override { _mm->setMemoryExecutable(); }
    void setPoisonMemory(bool poison) override { _mm->setPoisonMemory(poison); }

    uint8_t *startFunctionBody(const llvm::Function *F, uintptr_t &ActualSize) override { return _mm->startFunctionBody(F, ActualSize); }
    void endFunctionBody(const llvm::Function *F, uint8_t *FunctionStart, uint8_t *FunctionEnd) override { _mm->endFunctionBody(F, FunctionStart, FunctionEnd); }
    uint8_t* startExceptionTable(const llvm::Function* F, uintptr_t &ActualSize) override { return _mm->startExceptionTable(F, ActualSize); }
    void endExceptionTable(const llvm::Function *F, uint8_t *TableStart, uint8_t *TableEnd, uint8_t* FrameRegister) override { _mm->endExceptionTable(F, TableStart, TableEnd, FrameRegister); }
    void deallocateExceptionTable(void *ET) override { _mm->deallocateExceptionTable(ET); }

private:
    llvm::JITMemoryManager* _mm;
    const JIT::FunctionMapping* _functions = 0;
};

hilti::jit::MemoryManager::MemoryManager(llvm::JITMemoryManager *mm)
{
    _mm = mm;
}

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

    return _mm->getPointerToNamedFunction(name, abort_on_failure);
}

// An object cache that caches already JITed native code for later reuse.
class hilti::jit::ObjectCache : public llvm::ObjectCache
{
public:
    ObjectCache(CompilerContext* ctx);

    // Overriden from llvm::ObjectCache.
    void notifyObjectCompiled(const llvm::Module *module, const llvm::MemoryBuffer *obj) override;
    llvm::MemoryBuffer* getObject(const llvm::Module *module) override;

private:
    CompilerContext* _ctx;
    const llvm::Module* _key_module;
    ::util::cache::FileCache::Key _key;
};

hilti::jit::ObjectCache::ObjectCache(CompilerContext* ctx)
{
    _ctx = ctx;
    _key_module = 0;
}

void hilti::jit::ObjectCache::notifyObjectCompiled(const llvm::Module *module, const llvm::MemoryBuffer *obj)
{
    if ( ! _ctx->fileCache() )
        return;

    if ( _ctx->options().cgDebugging("cache" ) )
        std::cerr << util::fmt("Received compiled module %s for caching ...", module->getModuleIdentifier()) << std::endl;

    // Note that we can't just rebuild the key here. It seems the content of
    // module changes between the getObject() call and this method, so that
    // if we hash it now, we won't get matching results.

    assert(_key_module == module);

    if ( _key_module != module )
        abort();

    _ctx->fileCache()->store(_key, obj->getBufferStart(), obj->getBufferSize());
}

llvm::MemoryBuffer* hilti::jit::ObjectCache::getObject(const llvm::Module *module)
{
    if ( ! _ctx->fileCache() )
        return nullptr;

    ::util::cache::FileCache::Key key;
    key.scope = "a";
    key.name = module->getModuleIdentifier();
    _ctx->toCacheKey(module, &key);
    _ctx->options().toCacheKey(&key);

    _key_module = module;
    _key = key;

    auto lms = _ctx->fileCache()->lookup(key);
    assert(lms.size() <= 1);

    if ( lms.size() ) {
        if ( _ctx->options().cgDebugging("cache" ) )
            std::cerr << util::fmt("Reusing cached compiled module for %s.a", module->getModuleIdentifier()) << std::endl;

        return llvm::MemoryBuffer::getMemBufferCopy(lms.front());
    }

    if ( _ctx->options().cgDebugging("cache" ) )
        std::cerr << util::fmt("No cached compiled module for %s.a", module->getModuleIdentifier()) << std::endl;

    return nullptr;
}

JIT::JIT(CompilerContext* ctx)
{
    LLVMLinkInMCJIT();
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    _ctx = ctx;
    _mm = new MemoryManager(llvm::JITMemoryManager::CreateDefaultMemManager());
    _cache = new ObjectCache(ctx);
}

JIT::~JIT()
{
}

llvm::ExecutionEngine* JIT::jitModule(llvm::Module* module)
{
#if 0
    string err;
    llvm::raw_fd_ostream out("jitmodule.bc", err);
    llvm::WriteBitcodeToFile(module, out);
#endif

    auto opt = _ctx->options().optimize;
    auto opt_level = opt ? llvm::CodeGenOpt::Aggressive : llvm::CodeGenOpt::None;

    string errormsg;

    llvm::EngineBuilder builder(module);
    builder.setEngineKind(llvm::EngineKind::JIT);
    builder.setUseMCJIT(true);
    builder.setJITMemoryManager(_mm);
    builder.setErrorStr(&errormsg);
    builder.setOptLevel(opt_level);
    builder.setAllocateGVsWithCode(false);
    builder.setCodeModel(llvm::CodeModel::JITDefault);
    builder.setRelocationModel(llvm::Reloc::Default);
    builder.setMArch("");
    builder.setMCPU(llvm::sys::getHostCPUName());

    llvm::TargetOptions Options;
    Options.JITExceptionHandling = false;
    Options.JITEmitDebugInfo = true;
    Options.JITEmitDebugInfoToDisk = false;
    Options.UseSoftFloat = false;
    Options.FloatABIType = llvm::FloatABI::Default;
    Options.GuaranteedTailCallOpt = opt;
    // Options.PrintMachineCode = true;
    // Options.EnableSegmentedStacks = true; // Leads to "varargs not supported".
    builder.setTargetOptions(Options);

    auto ee = builder.create();
    if ( ! ee ) {
        error(util::fmt("LLVM jit error: %s", errormsg));
        return nullptr;
    }

    ee->DisableLazyCompilation(true);
    ee->runStaticConstructorsDestructors(false);
    ee->setObjectCache(_cache);

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

    return fp;
}

void JIT::installFunctionTable(const FunctionMapping* mappings)
{
    _mm->installFunctionTable(mappings);
}
