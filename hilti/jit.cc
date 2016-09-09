
// LLVM redefines the DEBUG macro. Sigh.
#ifdef DEBUG
#define __SAVE_DEBUG DEBUG
#undef DEBUG
#endif

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Transforms/Utils/Cloning.h"

#undef DEBUG
#ifdef __SAVE_DEBUG
#define DEBUG __SAVE_DEBUG
#endif

#include "codegen/common.h"
#include "jit.h"
#include "options.h"

#include <dlfcn.h>

extern "C" {
#include <libhilti/globals.h>
#include <libhilti/init.h>
}

using namespace hilti;

JIT::JIT(CompilerContext* ctx)
{
    _ctx = ctx;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto target = llvm::EngineBuilder().selectTarget();
    _target_machine.reset(target);

    _data_layout = std::make_unique<llvm::DataLayout>(_target_machine->createDataLayout());
    _object_layer = std::make_unique<ObjectLayer>();
    auto compiler = llvm::orc::SimpleCompiler(*_target_machine);
    _compile_layer = std::make_unique<CompileLayer>(*_object_layer, compiler);

    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

// Provide a weak default version of this libhilti function. This allows
// linking in the compiler library without also linking against the runtime,
// as long as one doesn't use anu JIT.
__attribute__((weak)) void* __hlt_runtime_state_get()
{
    fprintf(stderr, "executable has not been linked against libhilti, cannot use JIT\n");
    exit(1);
}

bool JIT::jit(std::unique_ptr<llvm::Module> module)
{
    // TODO: If we just JIT the module we have received, things will crash
    // badly once the module gets destroyed. Something like this:
    //
    //     While deleting: void (metadata, i64, metadata, metadata)* %llvm.dbg.value
    //     Use still stuck around after Def is destroyed:  tail call void @llvm.dbg.value(metadata
    //     i8* %ctx, i64 0, metadata <0x5b07fa0>, metadata <0x54902a0>), !dbg <0x5b07db8>
    //
    // I don't know if something's wrong with the module, or if that's a bug
    // in LLVM. Either way, as work-around, it works to move the module over
    // to a new context and then compile it there. We do this here first by
    // round-tripping through bitcode.

    llvm::SmallVector<char, 1024> buffer;
    llvm::raw_svector_ostream os(buffer);
    WriteBitcodeToFile(module.get(), os);

    llvm::LLVMContext jit_context;
    llvm::ObjectMemoryBuffer omb(std::move(buffer));
    auto module_clone = llvm::parseBitcodeFile(omb, jit_context);

    auto resolver = llvm::orc::createLambdaResolver(
        [&](const std::string& name) {
            // Symbol lookup inside the compiled code (is that right???)
            if ( auto sym = _compile_layer->findSymbol(name, false) )
                return sym.toRuntimeDyldSymbol();
            else
                return llvm::RuntimeDyld::SymbolInfo(nullptr);
        },

        [](const std::string& name) {
            // Symbol lookup inside the process.
            if ( auto symaddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name) )
                return llvm::RuntimeDyld::SymbolInfo(symaddr, llvm::JITSymbolFlags::Exported);
            else
                return llvm::RuntimeDyld::SymbolInfo(nullptr);
        });

    std::vector<std::unique_ptr<llvm::Module>> ms;
    ms.push_back(std::move(module_clone.get()));

    auto mm = std::make_unique<llvm::SectionMemoryManager>();
    _compile_layer->addModuleSet(std::move(ms), std::move(mm), std::move(resolver));

    // TODO: Looks like ORC doesn't have any error reporting yet.
    // https://llvm.org/bugs/show_bug.cgi?id=22612

    // Let the JIT use same global libhilti state as the current process
    // image. We use the current state so that any configuration informaiton
    // already set remains valid.
    //
    // We could achieve the mapping also through customizing the JIT's symbol
    // lookup, but this is more explicit.
    auto hrss = (void (*)(void*))nativeFunction("__hlt_runtime_state_set");

    (*hrss)(__hlt_runtime_state_get());

    auto jit_hlt_init = (void (*)())nativeFunction("__hlt_init");
    (*jit_hlt_init)();

    _jit_init();

    return true;
}

JIT::~JIT()
{
    auto jit_hlt_done = (void (*)())nativeFunction("__hlt_done");
    (*jit_hlt_done)();
}

void* JIT::nativeFunction(const string& function, bool must_exist)
{
    std::string mangled;
    llvm::raw_string_ostream mangled_stream(mangled);
    llvm::Mangler::getNameWithPrefix(mangled_stream, function, *_data_layout);

    if ( auto symbol = _compile_layer->findSymbol(mangled_stream.str(), true) )
        return (void*)symbol.getAddress();

    if ( ! must_exist )
        return nullptr;

    fatalError(::util::fmt("no function %s() in compiled module", function));
}

void JIT::_jit_init()
{
}
