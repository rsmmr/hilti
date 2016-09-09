
// TODO: The linker code is getting pretty messing. This needs cleanup/refactoring.

#include "linker.h"
#include "../context.h"
#include "../options.h"
#include "codegen.h"
#include "util.h"

using namespace hilti;
using namespace codegen;

// LLVM pass that replaces all calls to @hlt.globals.base.<Module> with code
// computing base address for the module's globals.
class GlobalsPass : public llvm::BasicBlockPass {
public:
    GlobalsPass(llvm::LLVMContext& ctx, ast::Logger* logger)
        : llvm::BasicBlockPass(ID), llvm_context(ctx), logger(logger)
    {
    }

    // Maps a module string to the GEP index for the the module's global
    // inside the joint struct.
    typedef std::map<std::string, std::pair<size_t, llvm::Type*>> globals_base_map;
    globals_base_map globals_base;

    // Maps a qualified global name to a module and the GEP index inside the
    // module's global struct.
    typedef std::map<std::string, std::pair<std::string, size_t>> globals_index_map;
    globals_index_map globals_index;

    llvm::LLVMContext& llvm_context;
    ast::Logger* logger;

    llvm::Type* globals_type;
    llvm::Type* execution_context_type;

    // Records which function we have already instrumented with code to
    // calculate the starts of the joined globals, inluding the value to
    // reference it.
    typedef std::map<llvm::Function*, llvm::Value*> function_map;
    function_map functions;

    bool runOnBasicBlock(llvm::BasicBlock& bb) override;

    void debug();

    const char* getPassName() const override
    {
        return "hilti::codegen::GlobalsPass";
    }

    static char ID;
};

char GlobalsPass::ID = 0;

void GlobalsPass::debug()
{
    logger->debug(1, ::util::fmt("final mapping of globals"));

    logger->debugPushIndent();

    for ( auto m : globals_base ) {
        string type;
        llvm::raw_string_ostream out(type);
        m.second.second->print(out);
        logger->debug(1, ::util::fmt("  module %s at global index %lu of type %s", m.first,
                                     m.second.first, out.str().c_str()));

        logger->debugPushIndent();

        for ( auto g : globals_index ) {
            auto id = g.first;
            auto module = g.second.first;
            auto idx = g.second.second;

            if ( module != m.first )
                continue;

            logger->debug(1, ::util::fmt("    %s at module index %lu", id, idx));
        }

        logger->debugPopIndent();
    }

    logger->debugPopIndent();
}

bool GlobalsPass::runOnBasicBlock(llvm::BasicBlock& bb)
{
    std::set<std::string> modules;

    // We can't replace the instruction while we're iterating over the block
    // so first identify and then replace afterwards.
    std::list<std::tuple<llvm::Instruction*, std::string>> replace;

    for ( auto ins = bb.begin(); ins != bb.end(); ++ins ) {
        auto md = ins->getMetadata("global-access");

        if ( ! md )
            continue;

        auto name = codegen::util::llvmMetadataAsString(md->getOperand(0));
        replace.push_back(std::make_tuple(&(*ins), name));
    }

    if ( ! replace.size() )
        return false;

    llvm::Value* base_addr = nullptr;
    llvm::Function* func = bb.getParent();

    auto f = functions.find(func);

    if ( f != functions.end() )
        base_addr = f->second;

    else {
        // Compute the base address for the joint globals struct inside the
        // execution context.

        // Get the execution context argument.
        llvm::Value* ectx = nullptr;

        for ( auto a = func->arg_begin(); a != func->arg_end(); ++a )
            if ( a->getName() == symbols::ArgExecutionContext )
                ectx = &(*a);

        if ( ! ectx )
            logger->internalError(
                "function accessing global does not have an execution context parameter");

        // Add instructions at the beginning of the function that give us the
        // address of the global variables inside the execution context.
        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvm_context, 32), 0);
        auto globals_idx =
            llvm::ConstantInt::get(llvm::Type::getIntNTy(llvm_context, 32), hlt::Globals);
        std::vector<llvm::Value*> indices;
        indices.push_back(zero);
        indices.push_back(globals_idx);

        auto i0 = llvm::CastInst::CreatePointerCast(ectx, execution_context_type);
        auto i1 = llvm::GetElementPtrInst::Create(nullptr, i0, indices, "");
        auto i2 = llvm::CastInst::CreatePointerCast(i1, llvm::PointerType::get(globals_type, 0));

        llvm::BasicBlock& entry_block = func->getEntryBlock();
        llvm::BasicBlock::InstListType& instrs = entry_block.getInstList();

        instrs.push_front(i2);
        instrs.push_front(i1);
        instrs.push_front(i0);

        base_addr = i2;
        functions.insert(std::make_pair(func, base_addr));
    }

    for ( auto i : replace ) {
        auto ins = std::get<0>(i);
        auto global = std::get<1>(i);

        auto g = globals_index.find(global);

        if ( g == globals_index.end() ) {
            logger->error(::util::fmt("reference to unknown global '%s'", global));
            continue;
        }

        auto module = g->second.first;
        auto midx = g->second.second;

        auto t = globals_base.find(module);

        if ( t == globals_base.end() ) {
            logger->error(
                ::util::fmt("reference to global '%s' in unknown module '%s'", global, module));
            continue;
        }

        auto gidx = t->second.first;

        // Generate an instruction that computes address of the desired
        // global by traversing the joint data structure appropiately.
        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvm_context, 32), 0);
        auto idx1 = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvm_context, 32), gidx);
        auto idx2 = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvm_context, 32), midx);

        std::vector<llvm::Value*> indices = {zero, idx1, idx2};
        auto gep = llvm::GetElementPtrInst::Create(nullptr, base_addr, indices, "");
        auto cgep = llvm::CastInst::CreatePointerCast(gep, ins->getType());

        // Finally replace the original instruction.
        //
        // TODO: I would think that the cast here shouldn't necessary,
        // however the linker seems to unify some named types, so that
        // suddently the pointer types don't match anymore. If so, we also
        // wouldn't need to insert 'gep' above, but just just that to replace
        // `ins` with.
        gep->insertBefore(ins);
        llvm::ReplaceInstWithInst(ins, cgep);
    }

    return true;
}

CompilerContext* Linker::context() const
{
    return _ctx;
}

llvm::LLVMContext& Linker::llvmContext()
{
    return _ctx->llvmContext();
}

const Options& Linker::options() const
{
    return _ctx->options();
}

void Linker::fatalError(const string& msg, const string& file, const string& error)
{
    string err = error.size() ? ::util::fmt(" (%s)", error) : "";

    if ( file.empty() )
        ast::Logger::fatalError(::util::fmt("error linking: %s", msg));
    else
        ast::Logger::fatalError(::util::fmt("error linking %s: %s%s", file, msg, err));
}

std::unique_ptr<llvm::Module> Linker::link(string output,
                                           std::list<std::unique_ptr<llvm::Module>>& modules)
{
    if ( options().cgDebugging("linker") )
        debugSetLevel(1);

    if ( modules.size() == 0 )
        fatalError("no modules given to linker");

    auto composite = std::make_unique<llvm::Module>(output, llvmContext());
    llvm::Linker linker(*composite);

    std::list<string> module_names;

    // Link in all the modules.

    for ( auto& i : modules ) {
        auto name = CodeGen::llvmGetModuleIdentifier(i.get());

        if ( isHiltiModule(i.get()) )
            module_names.push_back(name);

        linkInModule(&linker, std::move(i));

        if ( options().verify && ! codegen::util::llvmVerifyModule(composite.get()) )
            fatalError("verification failed", name);
    }

    // Link in bitcode libraries.

    for ( auto i : _bcs ) {
        string err;

        auto buffer = llvm::MemoryBuffer::getFile(i.c_str());

        if ( ! buffer )
            fatalError("reading bitcode failed", i);

        auto m = llvm::parseBitcodeFile(buffer.get().get()->getMemBufferRef(), llvmContext());

        if ( ! m )
            fatalError("parsing bitcode failed", i, m.getError().message());

        // The LLVM linker doesn't seem to pay attention to weak symbols: if
        // there's a weak function in a library it will overwrite any
        // function of the same name we already have, even if that has normal
        // linkage. So remove weak functions manually here.
        std::list<llvm::Function*> to_remove;

        for ( auto& f : (*m)->functions() ) {
            if ( f.hasWeakLinkage() && composite->getFunction(f.getName()) ) {
                to_remove.push_back(&f);
            }
        }

        for ( auto f : to_remove )
            f->removeFromParent();

        linkInModule(&linker, std::move(*m));
    }

    // Link native library.

    for ( auto i : _natives )
        linkInNativeLibrary(&linker, i);

    if ( module_names.size() ) {
        addModuleInfo(composite.get(), module_names);
        addGlobalsInfo(composite.get(), module_names);
        makeHooks(composite.get(), module_names);
    }

    if ( errors() > 0 )
        return nullptr;

    return composite;
}

bool Linker::isHiltiModule(llvm::Module* module)
{
    string id = CodeGen::llvmGetModuleIdentifier(module);
    string name = string(symbols::MetaModule) + "." + id;

    auto md = codegen::util::llvmGetGlobalMetadata(module, name);
    bool is_hilti = (md != nullptr);

    if ( is_hilti )
        debug(1, ::util::fmt("module %s has HILTI meta data", id.c_str()));
    else
        debug(1, ::util::fmt("module %s does not have HILTI meta data", id.c_str()));

    return is_hilti;
}

void Linker::joinFunctions(llvm::Module* dst, const char* new_func, const char* meta,
                           llvm::FunctionType* default_ftype, const std::list<string>& module_names,
                           llvm::Module* module)
{
    IRBuilder* builder = nullptr;
    llvm::Function* nfunc = nullptr;

    auto md = codegen::util::llvmGetGlobalMetadata(module, meta);

    if ( ! md )
        return;

    // If a function under that name already exists with weak linkage,
    // replace it.
    auto old_func = dst->getFunction(new_func);

    if ( old_func ) {
        old_func->removeFromParent();
    }

    for ( int i = 0; i < md->getNumOperands(); ++i ) {
        auto op = md->getOperand(i);
        for ( int j = 0; j < op->getNumOperands(); ++j ) {
            auto func = codegen::util::llvmMetadataAsValue(op->getOperand(j));

            if ( ! builder ) {
                // Create new function with same signature.
                auto ftype_ptr = llvm::cast<llvm::PointerType>(func->getType());
                auto old_ftype = llvm::cast<llvm::FunctionType>(ftype_ptr->getElementType());
                std::vector<llvm::Type*> params(old_ftype->param_begin(), old_ftype->param_end());
                auto ftype = llvm::FunctionType::get(old_ftype->getReturnType(), params, false);
                nfunc =
                    llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, new_func, dst);
                nfunc->setCallingConv(llvm::CallingConv::C);
                builder =
                    codegen::util::newBuilder(llvmContext(),
                                              llvm::BasicBlock::Create(llvmContext(), "", nfunc));

                if ( old_func )
                    old_func->replaceAllUsesWith(nfunc);
            }

            std::vector<llvm::Value*> params;
            int n = 1;
            for ( auto a = nfunc->arg_begin(); a != nfunc->arg_end(); ++a ) {
                a->setName(::util::fmt("__arg%d", n++));
                params.push_back(&(*a));
            }

            codegen::util::checkedCreateCall(builder, "Linker", func, params);
        }
    }

    assert(builder);
    builder->CreateRetVoid();
}

struct HookImpl {
    llvm::Value* func;
    int64_t priority;
    int64_t group;

    // Inverse sort.
    bool operator<(const HookImpl& other) const
    {
        return priority > other.priority;
    }
};

struct HookDecl {
    llvm::Value* func;
    llvm::Type* result;
    std::vector<HookImpl> impls;
};

static void _debugDumpHooks(Linker* linker, const std::map<string, HookDecl>& hooks)
{
    for ( auto h : hooks ) {
        linker->debug(1, ::util::fmt("hook %s - Trigger function '%s'", h.first.c_str(),
                                     h.second.func->getName().str().c_str()));
        for ( auto i : h.second.impls )
            linker->debug(1, ::util::fmt("   -> '%s' (priority %" PRId64 ", group %" PRId64 ")",
                                         i.func->getName().str().c_str(), i.priority, i.group));
    }
}

void Linker::makeHooks(llvm::Module* module, const std::list<string>& module_names)
{
    auto decls = codegen::util::llvmGetGlobalMetadata(module, symbols::MetaHookDecls);
    auto impls = codegen::util::llvmGetGlobalMetadata(module, symbols::MetaHookImpls);

    if ( ! decls ) {
        debug(1, "no hooks declared in any module");
        return;
    }

    std::map<string, HookDecl> hooks;

    // Collects all the declarations first.
    for ( int i = 0; i < decls->getNumOperands(); ++i ) {
        auto entry = codegen::util::llvmMetadataAsTuple(decls->getOperand(i));
        auto name = codegen::util::llvmMetadataAsString(entry->getOperand(0));
        auto func =
            llvm::cast<llvm::Function>(codegen::util::llvmMetadataAsValue(entry->getOperand(1)));
        auto result =
            entry->getNumOperands() > 2 ? util::llvmMetadataAsValue(entry->getOperand(2)) : nullptr;

        if ( ! func || func->isNullValue() )
            // Optimization may throw out the function if it's not
            // used/empty, in which case the pointer in the meta-data will be
            // set to null.
            continue;

        HookDecl decl;
        decl.func = func;
        decl.result = result ? result->getType() : nullptr;

        auto h = hooks.find(name);

        if ( h == hooks.end() )
            hooks.insert(std::make_pair(name, decl));

        else {
            // TODO: Already declared, should make sure it matches.
        }
    }

    if ( impls ) {
        // Now add the implementations.
        for ( int i = 0; i < impls->getNumOperands(); ++i ) {
            auto entry = codegen::util::llvmMetadataAsTuple(impls->getOperand(i));
            auto name = codegen::util::llvmMetadataAsString(entry->getOperand(0));
            auto func = llvm::cast<llvm::Function>(
                codegen::util::llvmMetadataAsValue(entry->getOperand(1)));
            auto priority =
                llvm::cast<llvm::Value>(codegen::util::llvmMetadataAsValue(entry->getOperand(2)));
            auto group =
                llvm::cast<llvm::Value>(codegen::util::llvmMetadataAsValue(entry->getOperand(3)));

            auto h = hooks.find(name);

            if ( h == hooks.end() )
                // Hook isn't declared, which means it will never be called.
                continue;

            HookImpl impl;
            impl.func = llvm::cast<llvm::Function>(func);
            impl.priority = llvm::cast<llvm::ConstantInt>(priority)->getSExtValue();
            impl.group = llvm::cast<llvm::ConstantInt>(group)->getSExtValue();

            (*h).second.impls.push_back(impl);
        }
    }

    // Reverse sort the implementation by priority.
    for ( auto h = hooks.begin(); h != hooks.end(); ++h ) {
        std::sort((*h).second.impls.begin(), (*h).second.impls.end());
    }

    _debugDumpHooks(this, hooks);

    // Finally, we can build the functions that call the hook implementations.

    auto true_ = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmContext(), 1), 1);
    auto false_ = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmContext(), 1), 0);

    for ( auto h : hooks ) {
        auto decl = h.second;

        // Add a body to the entry function. The body calls each function
        // successively as long as none return true as an indication that it
        // wants to stop hook execution.
        auto func = llvm::cast<llvm::Function>(decl.func);
        auto stopped =
            codegen::util::newBuilder(llvmContext(),
                                      llvm::BasicBlock::Create(llvmContext(), "stopped"));
        auto next =
            codegen::util::newBuilder(llvmContext(),
                                      llvm::BasicBlock::Create(llvmContext(), "hook", func));

        for ( auto i : decl.impls ) {
            auto ifunc = llvm::cast<llvm::Function>(i.func);

            IRBuilder* current = next;
            next = codegen::util::newBuilder(llvmContext(),
                                             llvm::BasicBlock::Create(llvmContext(), "hook", func));

            std::vector<llvm::Value*> args;

            auto pt = ifunc->arg_begin();
            for ( auto p = func->arg_begin(); p != func->arg_end(); ++p ) {
                auto casted = current->CreateBitCast(&(*p), pt->getType());
                args.push_back(casted);
                pt++;
            }

            auto result =
                codegen::util::checkedCreateCall(current, "Linker::makeHooks", ifunc, args);

            llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmContext(), 1), 0);
            current->CreateCondBr(result, stopped->GetInsertBlock(), next->GetInsertBlock());
        }

        next->CreateRet(false_);
        stopped->CreateRet(true_);
        func->getBasicBlockList().push_back(stopped->GetInsertBlock());
    }
}

void Linker::addModuleInfo(llvm::Module* module, const std::list<string>& module_names)
{
    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(llvmContext(), 8), 0);
    std::vector<llvm::Type*> args = {voidp};
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext()), args, false);

    joinFunctions(module, symbols::FunctionModulesInit, symbols::MetaModuleInit, ftype,
                  module_names, module);
}

void Linker::addGlobalsInfo(llvm::Module* module, const std::list<string>& module_names)
{
    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(llvmContext(), 8), 0);
    std::vector<llvm::Type*> args = {voidp};
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext()), args, false);

    joinFunctions(module, symbols::FunctionGlobalsInit, symbols::MetaGlobalsInit, ftype,
                  module_names, module);
    joinFunctions(module, symbols::FunctionGlobalsDtor, symbols::MetaGlobalsDtor, ftype,
                  module_names, module);

    // Create the joint globals type and determine its size, and record the indices for the
    // individual modules.

    std::vector<llvm::Type*> globals;
    GlobalsPass::globals_base_map globals_base;

    for ( auto name : module_names ) {
        // Add module's globals to the joint type and keep track of the GEP
        // index in there.
        auto type = module->getTypeByName(symbols::TypeGlobals + string(".") + name);

        if ( ! type )
            continue;

        globals_base.insert(std::make_pair(name, std::make_pair(globals.size(), type)));
        globals.push_back(type);
    }

    // Create the joint globals type.
    auto ty_globals = llvm::StructType::create(llvmContext(), globals, symbols::TypeGlobals);

    // Create the function returning the size of the globals struct, using the "portable sizeof"
    // idiom ...
    auto null = llvm::Constant::getNullValue(llvm::PointerType::get(ty_globals, 0));
    auto one = llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmContext(), 32), 1);
    auto addr = llvm::ConstantExpr::getGetElementPtr(nullptr, null, one);
    auto size = llvm::ConstantExpr::getPtrToInt(addr, llvm::Type::getIntNTy(llvmContext(), 64));

    // If a global_sizes() function already exists with weak linkage, replace
    // it.
    auto old_func = module->getFunction(symbols::FunctionGlobalsSize);

    if ( old_func ) {
        old_func->removeFromParent();
    }

    auto gftype = llvm::FunctionType::get(llvm::Type::getIntNTy(llvmContext(), 64), false);
    auto gfunc = llvm::Function::Create(gftype, llvm::Function::ExternalLinkage,
                                        symbols::FunctionGlobalsSize, module);
    gfunc->setCallingConv(llvm::CallingConv::C);
    auto builder = codegen::util::newBuilder(llvmContext(),
                                             llvm::BasicBlock::Create(llvmContext(), "", gfunc));
    builder->CreateRet(size);

    if ( old_func )
        old_func->replaceAllUsesWith(gfunc);

    // Create a map of all global IDs to the GEP index inside their
    // individual global structs.
    GlobalsPass::globals_index_map globals_index;

    for ( auto m : module_names ) {
        auto name = symbols::MetaGlobals + string(".") + m;
        auto md = codegen::util::llvmGetGlobalMetadata(module, name);

        if ( ! md )
            continue;

        for ( int i = 0; i < md->getNumOperands(); i++ ) {
            int cnt = 0;
            auto tuple = codegen::util::llvmMetadataAsTuple(md->getOperand(i));
            for ( int j = 0; j < tuple->getNumOperands(); j++ ) {
                auto global = codegen::util::llvmMetadataAsString(tuple->getOperand(j));
                auto k = globals_index.find(global);

                if ( k != globals_index.end() )
                    fatalError(::util::fmt("global %s defined in two compilation units (%s and %s)",
                                           global, name, k->second.first));

                globals_index.insert(std::make_pair(global, std::make_pair(m, cnt++)));
            }
        }
    }

    // Now run a pass over all the modules replacing the dummy accesses to globals with
    // the actual version.
    auto pass = new GlobalsPass(llvmContext(), this);
    pass->globals_base = globals_base;
    pass->globals_index = globals_index;
    pass->globals_type = ty_globals;
    pass->debug();

    string name = string(symbols::MetaModule) + "." + module_names.front();
    auto md = codegen::util::llvmGetGlobalMetadata(module, name)->getOperand(0);
    auto ectx =
        codegen::util::llvmMetadataAsValue(md->getOperand(symbols::MetaModuleExecutionContextType));

    pass->execution_context_type = ectx->getType();

    llvm::legacy::PassManager mgr;
    mgr.add(pass);
    mgr.run(*module);
}

void hilti::codegen::Linker::linkInModule(llvm::Linker* linker,
                                          std::unique_ptr<llvm::Module> module)
{
    auto name = CodeGen::llvmGetModuleIdentifier(module.get());

    debug(1, ::util::fmt("linking in module %s", name));

    if ( linker->linkInModule(std::move(module), llvm::Linker::OverrideFromSrc) )
        fatalError("module linking error", name);
}

void hilti::codegen::Linker::linkInNativeLibrary(llvm::Linker* linker, const string& library)
{
    fatalError("native libraries not supported currently", library);
}
