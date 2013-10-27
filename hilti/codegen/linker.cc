
// TODO: The linker code is getting pretty messing. This needs cleanup/refactoring.

#include "linker.h"
#include "util.h"
#include "codegen.h"
#include "../options.h"
#include "../context.h"

using namespace hilti;
using namespace codegen;

// LLVM pass that replaces all calls to @hlt.globals.base.<Module> with code
// computing base address for the module's globals.
class GlobalsPass : public llvm::BasicBlockPass
{
public:
   GlobalsPass(ast::Logger* logger) : llvm::BasicBlockPass(ID), logger(logger) {}

   // Maps a module string to the GEP index for the the module's global
   // inside the joint struct.
   typedef std::map<std::string, std::pair<size_t, llvm::Type*>> globals_base_map;
   globals_base_map globals_base;

   // Maps a qualified global name to the GEP index inside the module's global struct.
   typedef std::map<std::string, size_t> globals_index_map;
   globals_index_map globals_index;

   ast::Logger* logger;

   llvm::Type* globals_type;
   llvm::Type* execution_context_type;

   // Records which function we have already instrumented with code to
   // calculate the starts of the joined globals, inluding the value to
   // reference it.
   typedef std::map<llvm::Function*, llvm::Value *> function_map;
   function_map functions;


   bool runOnBasicBlock(llvm::BasicBlock &bb) override;

   void debug();

   const char* getPassName() const override { return "hilti::codegen::GlobalsPass"; }

   static char ID;

};

char GlobalsPass::ID = 0;

void GlobalsPass::debug()
{
    logger->debug(1, ::util::fmt("final mapping of globals"));

    logger->debugPushIndent();

    for ( auto m : globals_base ) {
        logger->debug(1, ::util::fmt("  module %s at global index %lu of type %s", m.first, m.second.first, m.second.second));

        logger->debugPushIndent();

        for ( auto g : globals_index ) {
            auto i = ::util::strsplit(g.first, "::");

            if ( i.front() != ::util::strtolower(m.first) )
                continue;

            logger->debug(1, ::util::fmt("    %s at module index %lu", g.first, g.second));
        }

        logger->debugPopIndent();
    }

    logger->debugPopIndent();

}

bool GlobalsPass::runOnBasicBlock(llvm::BasicBlock &bb)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    std::set<std::string> modules;

    // We can't replace the instruction while we're iterating over the block
    // so first identify and then replace afterwards.
    std::list<std::tuple<llvm::Instruction *, std::string, std::string>> replace;

    for ( auto ins = bb.begin(); ins != bb.end(); ++ins ) {
        auto md = ins->getMetadata("global-access");

        if ( ! md )
            continue;

        auto module = llvm::cast<llvm::MDString>(md->getOperand(0))->getString();
        auto global = llvm::cast<llvm::MDString>(md->getOperand(1))->getString();

        replace.push_back(std::make_tuple(ins, module, global));
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
                ectx = a;

        if ( ! ectx )
            logger->internalError("function accessing global does not have an execution context parameter");

        // Add instructions at the beginning of the function that give us the
        // address of the global variables inside the execution context.
        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 0);
        auto globals_idx = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), hlt::Globals);
        std::vector<llvm::Value*> indices;
        indices.push_back(zero);
        indices.push_back(globals_idx);

        auto i0 = llvm::CastInst::CreatePointerCast(ectx, execution_context_type);
        auto i1 = llvm::GetElementPtrInst::Create(i0, indices, "");
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
        auto module = ::util::strtolower(std::get<1>(i));
        auto global = std::get<2>(i);

        // std::cerr << module << " / " << global << std::endl;

        // Generate an instruction that computes address of the desired
        // global by traversing the joint data structure appropiately.

        auto t = globals_base.find(module);

        if ( t == globals_base.end() ) {
            logger->error(::util::fmt("reference to global '%s' in unknown module '%s'", global, std::get<1>(i)));
            continue;
        }

        auto gidx = (*t).second.first;
        auto gtype = (*t).second.second;

        auto id = ::util::fmt("%s::%s", module, global);
        auto m = globals_index.find(id);

        if ( m == globals_index.end() ) {
            logger->error(::util::fmt("reference to unknown global '%s::%s'", std::get<1>(i), global));
            continue;
        }

        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 0);
        auto idx1 = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), gidx);
        auto idx2 = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), m->second);

        std::vector<llvm::Value*> indices = { zero, idx1, idx2 };
        auto gep = llvm::GetElementPtrInst::Create(base_addr, indices, "");

        // Finally replace the original instruction.

        // std::cerr << "replacing ";
        // ins->dump();
        // std::cerr << "with      ";
        // gep->dump();

        llvm::ReplaceInstWithInst(ins, gep);
    }

    return true;
}

CompilerContext* Linker::context() const
{
    return _ctx;
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

llvm::Module* Linker::link(string output, const std::list<llvm::Module*>& modules)
{
    if ( options().cgDebugging("linker") )
        debugSetLevel(1);

    if ( modules.size() == 0 )
        fatalError("no modules given to linker");

    llvm::Module* composite = new llvm::Module(output, llvm::getGlobalContext());
    llvm::Linker linker(composite);

    std::list<string> module_names;

    // Link in all the modules.

    for ( auto i : modules ) {

        auto name = CodeGen::llvmGetModuleIdentifier(i);

        if ( isHiltiModule(i) )
            module_names.push_back(name);

        linkInModule(&linker, i);

        if ( options().verify && ! util::llvmVerifyModule(linker.getModule()) )
            fatalError("verification failed", name);
    }

    // Link in bitcode libraries.

    for ( auto i : _bcs ) {
        string err;

        llvm::OwningPtr<llvm::MemoryBuffer> buffer;

        if ( llvm::MemoryBuffer::getFile(i.c_str(), buffer) )
            fatalError("reading bitcode failed", i);

        llvm::Module* bc = llvm::ParseBitcodeFile(buffer.get(), llvm::getGlobalContext(), &err);

        if ( ! bc )
            fatalError("parsing bitcode failed", i, err);

        linkInModule(&linker, bc);
    }

    // Link native library.

    for ( auto i : _natives )
        linkInNativeLibrary(&linker, i);

    if ( module_names.size() ) {
        // In LLVM <= 3.3, we need to add these to a separate module that we
        // then link in; if we added it to the module directly, names
        // wouldn't be unified correctly. Starting with LLVM 3.4, we can add
        // it to the composite module directly.
#ifdef HAVE_LLVM_33
        auto target_module = new ::llvm::Module("__linker_stuff", llvm::getGlobalContext());
#else
        auto target_module = linker.getModule();
#endif
        addModuleInfo(target_module, module_names, linker.getModule());
        addGlobalsInfo(target_module, module_names, linker.getModule());

#ifdef HAVE_LLVM_33
        linkInModule(&linker, target_module);
#endif

        makeHooks(module_names, linker.getModule());
    }

    if ( errors() > 0 )
        return nullptr;

    return linker.getModule();
}

bool Linker::isHiltiModule(llvm::Module* module)
{
    string id = CodeGen::llvmGetModuleIdentifier(module);
    string name = string(symbols::MetaModule) + "." + id;

    auto md = module->getNamedMetadata(name);
    bool is_hilti = (md != nullptr);

    if ( is_hilti )
        debug(1, ::util::fmt("module %s has HILTI meta data", id.c_str()));
    else
        debug(1, ::util::fmt("module %s does not have HILTI meta data", id.c_str()));

    return is_hilti;
}

llvm::NamedMDNode* Linker::moduleMeta(llvm::Module* module, shared_ptr<ID> id)
{
    string name = string(symbols::MetaModule) + "." + id->name();
    auto md = module->getNamedMetadata(name);
    assert(md);
    return md;
}

void Linker::joinFunctions(llvm::Module* dst, const char* new_func, const char* meta, llvm::FunctionType* default_ftype, const std::list<string>& module_names, llvm::Module* module)
{
    auto md = module->getNamedMetadata(meta);

    IRBuilder* builder = nullptr;
    llvm::LLVMContext& ctx = llvm::getGlobalContext();
    llvm::Function* nfunc = nullptr;

    if ( ! md )
        return;

#if 0
    if ( ! md ) {
        // No such meta node, i.e., no module defines the function
        //
        // Create a function with the given type so that it exists, even if
        // empty. In this case, the function type doesn't need to have the
        // exact types as long as the cc does the right thing.
        nfunc = llvm::Function::Create(default_ftype, llvm::Function::ExternalLinkage, new_func, dst);
        nfunc->setCallingConv(llvm::CallingConv::C);
        builder = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "", nfunc));
        builder->CreateRetVoid();
        return;
    }
#endif

    for ( int i = 0; i < md->getNumOperands(); ++i ) {
        auto node = llvm::cast<llvm::MDNode>(md->getOperand(i));
        auto func = llvm::cast<llvm::Function>(node->getOperand(0));

        if ( ! builder ) {
            // Create new function with same signature.
            auto ftype_ptr = llvm::cast<llvm::PointerType>(func->getType());
            auto old_ftype = llvm::cast<llvm::FunctionType>(ftype_ptr->getElementType());
            std::vector<llvm::Type*> params(old_ftype->param_begin(), old_ftype->param_end());
            auto ftype = llvm::FunctionType::get(old_ftype->getReturnType(), params, false);
            nfunc = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, new_func, dst);
            nfunc->setCallingConv(llvm::CallingConv::C);
            builder = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "", nfunc));
        }

        std::vector<llvm::Value*> params;
        int n = 1;
        for ( auto a = nfunc->arg_begin(); a != nfunc->arg_end(); ++a ) {
            a->setName(::util::fmt("__arg%d", n++));
            params.push_back(a);
        }

        util::checkedCreateCall(builder, "Linker", func, params);
    }

    builder->CreateRetVoid();
}

struct HookImpl {
    llvm::Value* func;
    int64_t priority;
    int64_t group;

    // Inverse sort.
    bool operator<(const HookImpl& other) const { return priority > other.priority; }
};

struct HookDecl {
    llvm::Value* func;
    llvm::Type* result;
    std::vector<HookImpl> impls;
};

static void _debugDumpHooks(Linker* linker, const std::map<string, HookDecl>& hooks)
{
    for ( auto h : hooks ) {
        linker->debug(1, ::util::fmt("hook %s - Trigger function '%s'", h.first.c_str(), h.second.func->getName().str().c_str()));
        for ( auto i : h.second.impls )
            linker->debug(1, ::util::fmt("   -> '%s' (priority %" PRId64 ", group %" PRId64 ")",
                                         i.func->getName().str().c_str(), i.priority, i.group));
    }
}

void Linker::makeHooks(const std::list<string>& module_names, llvm::Module* module)
{
    auto decls = module->getNamedMetadata(symbols::MetaHookDecls);
    auto impls = module->getNamedMetadata(symbols::MetaHookImpls);

    if ( ! decls ) {
        debug(1, "no hooks declared in any module");
        return;
    }

    std::map<string, HookDecl> hooks;

    // Collects all the declarations first.
    for ( int i = 0; i < decls->getNumOperands(); ++i ) {
        HookDecl decl;

        auto node = llvm::cast<llvm::MDNode>(decls->getOperand(i));
        auto name = llvm::cast<llvm::MDString>(node->getOperand(0))->getString();

        auto op1 = llvm::cast_or_null<llvm::Constant>(node->getOperand(1));

        if ( ! op1 || op1->isNullValue() )
            // Optimization may throw out the function if it's not
            // used/empty, in which case the pointer in the meta-data will be
            // set to null.
            continue;

        decl.func = llvm::cast<llvm::Function>(op1);

        if ( node->getNumOperands() > 2 )
            decl.result = node->getOperand(2)->getType();
        else
            decl.result = nullptr;

        auto h = hooks.find(name);

        if ( h == hooks.end() )
            hooks.insert(std::make_pair(name, decl));

        else {
            // Already declared, make sure it matches.
            auto other = (*h).second;

            //if ( decl.func != other.func || decl.result != other.result )
            //    fatalError(::util::fmt("inconsistent declarations found for hook %s", name.str().c_str()));
        }
    }

    if ( impls ) {

        // Now add the implementations.
        for ( int i = 0; i < impls->getNumOperands(); ++i ) {
            auto node = llvm::cast<llvm::MDNode>(impls->getOperand(i));
            auto name = llvm::cast<llvm::MDString>(node->getOperand(0))->getString();

            auto h = hooks.find(name);

            if ( h == hooks.end() )
                // Hook isn't declared, which means it will never be called.
                continue;

            HookImpl impl;
            impl.func = llvm::cast<llvm::Function>(node->getOperand(1));
            impl.priority = llvm::cast<llvm::ConstantInt>(node->getOperand(2))->getSExtValue();
            impl.group = llvm::cast<llvm::ConstantInt>(node->getOperand(3))->getSExtValue();

            (*h).second.impls.push_back(impl);
        }

    }

    // Reverse sort the implementation by priority.
    for ( auto h = hooks.begin(); h != hooks.end(); ++h ) {
        std::sort((*h).second.impls.begin(), (*h).second.impls.end());
    }

    _debugDumpHooks(this, hooks);

    // Finally, we can build the functions that call the hook implementations.

    llvm::LLVMContext& ctx = llvm::getGlobalContext();
    auto true_ = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 1), 1);
    auto false_ = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 1), 0);

    for ( auto h : hooks ) {
        auto decl = h.second;

        // Add a body to the entry function. The body calls each function
        // successively as long as none return true as an indication that it
        // wants to stop hook execution.
        auto func = llvm::cast<llvm::Function>(decl.func);
        auto stopped = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "stopped"));
        auto next = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "hook", func));

        for ( auto i : decl.impls ) {
            IRBuilder* current = next;
            next = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "hook", func));

            std::vector<llvm::Value *> args;

            for ( auto p = func->arg_begin(); p != func->arg_end(); ++p )
                args.push_back(p);

            auto result = util::checkedCreateCall(current, "Linker::makeHooks", i.func, args);

            llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 1), 0);
            current->CreateCondBr(result, stopped->GetInsertBlock(), next->GetInsertBlock());
        }

        next->CreateRet(false_);
        stopped->CreateRet(true_);
        func->getBasicBlockList().push_back(stopped->GetInsertBlock());
    }
}

void Linker::addModuleInfo(llvm::Module* dst, const std::list<string>& module_names, llvm::Module* module)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(ctx, 8), 0);
    std::vector<llvm::Type*> args = { voidp };
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), args, false);

    joinFunctions(dst, symbols::FunctionModulesInit, symbols::MetaModuleInit, ftype, module_names, module);
}

void Linker::addGlobalsInfo(llvm::Module* dst, const std::list<string>& module_names, llvm::Module* module)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(ctx, 8), 0);
    std::vector<llvm::Type*> args = { voidp };
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), args, false);

    joinFunctions(dst, symbols::FunctionGlobalsInit, symbols::MetaGlobalsInit, ftype, module_names, module);
    joinFunctions(dst, symbols::FunctionGlobalsDtor, symbols::MetaGlobalsDtor, ftype, module_names, module);

    // Create the joint globals type and determine its size, and record the indices for the individual modules.

    std::vector<llvm::Type*> globals;
    GlobalsPass::globals_base_map globals_base;

    for ( auto name : module_names ) {
        // Add module's globals to the joint type and keep track of the GEP index in there.
        auto type = module->getTypeByName(symbols::TypeGlobals + string(".") + name);

        if ( ! type )
            continue;

        globals_base.insert(std::make_pair(::util::strtolower(name), std::make_pair(globals.size(), type)));
        globals.push_back(type);
    }

    // Create the joint globals type.
    auto ty_globals = llvm::StructType::create(ctx, globals, symbols::TypeGlobals);

    // Create the function returning the size of the globals struct, using the "portable sizeof" idiom ...
    auto null = llvm::Constant::getNullValue(llvm::PointerType::get(ty_globals, 0));
    auto one = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 1);
    auto addr = llvm::ConstantExpr::getGetElementPtr(null, one);
    auto size = llvm::ConstantExpr::getPtrToInt(addr, llvm::Type::getIntNTy(ctx, 64));

    auto gftype = llvm::FunctionType::get(llvm::Type::getIntNTy(ctx, 64), false);
    auto gfunc = llvm::Function::Create(gftype, llvm::Function::ExternalLinkage, symbols::FunctionGlobalsSize, dst);
    gfunc->setCallingConv(llvm::CallingConv::C);
    auto builder = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "", gfunc));
    builder->CreateRet(size);

    // Create a map of all global IDs to the GEP index inside their individual global structs.
    GlobalsPass::globals_index_map globals_index;

    for ( auto name : module_names ) {
        auto md = module->getNamedMetadata(symbols::MetaGlobals + string(".") + name);

        if ( ! md )
            continue;

        for ( int i = 0; i < md->getNumOperands(); i++ ) {
            auto node = llvm::cast<llvm::MDNode>(md->getOperand(i));
            auto global = llvm::cast<llvm::MDString>(node->getOperand(0))->getString().str();
            auto id = ::util::fmt("%s::%s", ::util::strtolower(name), global);
            globals_index.insert(std::make_pair(id, i));
        }
    }

    // Now run a pass over all the modules replacing the dummy accesses to globals with
    // the actual version.
    auto pass = new GlobalsPass(this);
    pass->globals_base = globals_base;
    pass->globals_index = globals_index;
    pass->globals_type = ty_globals;
    pass->debug();

    auto md = Linker::moduleMeta(module, std::make_shared<hilti::ID>(module_names.front()));
    assert(md);
    pass->execution_context_type = md->getOperand(symbols::MetaModuleExecutionContextType)->getOperand(0)->getType();

    llvm::PassManager mgr;
    mgr.add(pass);
    mgr.run(*module);
}

void hilti::codegen::Linker::linkInModule(llvm::Linker* linker, llvm::Module* module)
{
    auto name = CodeGen::llvmGetModuleIdentifier(module);

    debug(1, ::util::fmt("linking in module %s", name));

    string err;

    if ( linker->linkInModule(module, &err) )
        fatalError("module linking error", name, err);
}

void hilti::codegen::Linker::linkInNativeLibrary(llvm::Linker* linker, const string& library)
{
    fatalError("native libraries not supported currently", library);
}

