
#include "linker.h"
#include "util.h"
#include "codegen.h"

// TODO: THis needs cleanup.

using namespace hilti;
using namespace codegen;

// LLVM pass that replaces all calls to @hlt.globals.base.<Module> with code
// computing base address for the module's globals.
class GlobalsBasePass : public llvm::BasicBlockPass
{
public:
   GlobalsBasePass() : llvm::BasicBlockPass(ID) {}

   // Maps @hlt.globals.base.<Module> functions to the base index in the joint globals struct.
   typedef std::map<llvm::Value *, llvm::Value *> transform_map;
   transform_map tmap;
   llvm::Type* globals_type;

   // Records which function we have already instrumented with code to
   // calculate the start of our globals, inluding the value to reference it.
   typedef std::map<llvm::Function*, llvm::Value *> function_set;
   function_set functions;

   bool runOnBasicBlock(llvm::BasicBlock &bb) override;

   const char* getPassName() const override { return "hilti::codegen::GlobalsBasePass"; }

   static char ID;

};

char GlobalsBasePass::ID = 0;

bool GlobalsBasePass::runOnBasicBlock(llvm::BasicBlock &bb)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    // We can't replace the instruction while we're iterating over the block
    // so first identify and then replace afterwards.
    std::list<std::pair<llvm::CallInst *, llvm::Value *>> replace;

    for ( auto i = bb.begin(); i != bb.end(); ++i ) {
        auto call = llvm::dyn_cast<llvm::CallInst>(i);
        if ( ! call )
            continue;

        auto j = tmap.find(call->getCalledFunction());

        if ( j != tmap.end() )
            replace.push_back(std::make_pair(call, j->second));
    }

    if ( ! replace.size() )
        return false;

    llvm::Value* base_addr = nullptr;
    llvm::Function* func = bb.getParent();

    auto f = functions.find(func);

    if ( f != functions.end() )
        base_addr = f->second;

    else {
        // Get the execution context argument.
        llvm::Value* ectx = nullptr;

        for ( auto a = func->arg_begin(); a != func->arg_end(); ++a )
            if ( a->getName() == symbols::ArgExecutionContext )
                ectx = a;

        if ( ! ectx )
            throw InternalError("function accessing global does not have an execution context parameter");

        // Add instructions at the beginning of the function that give us the
        // address of the global variables inside the execution context.
        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 0);
        auto globals_idx = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), hlt::Globals);
        std::vector<llvm::Value*> indices;
        indices.push_back(zero);
        indices.push_back(globals_idx);

        auto i1 = llvm::GetElementPtrInst::Create(ectx, indices, "");
        auto i2 = llvm::CastInst::CreatePointerCast(i1, llvm::PointerType::get(globals_type, 0));

        llvm::BasicBlock& entry_block = func->getEntryBlock();
        llvm::BasicBlock::InstListType& instrs = entry_block.getInstList();

        instrs.push_front(i2);
        instrs.push_front(i1);

        functions.insert(std::make_pair(func, i2));
        base_addr = i2;
    }

    // Now replace the fake calls with the actual address calculation.
    for ( auto i : replace ) {
        auto zero = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 0);

        std::vector<llvm::Value*> indices;
        indices.push_back(zero);
        indices.push_back(i.second);
        auto gep = llvm::GetElementPtrInst::Create(base_addr, indices, "");

        llvm::ReplaceInstWithInst(i.first, gep);
    }

    return true;
}

void Linker::error(const llvm::Linker& linker, const string& where, const string& file, const string& error)
{
    string err = error.size() ? error : linker.getLastError();

    ast::Logger::error(::util::fmt("error linking %s in %s: %s", file.c_str(), where.c_str(), err.c_str()));
}

llvm::Module* Linker::link(string output, const std::list<llvm::Module*>& modules, bool verify, int debug_cg)
{
    debugSetLevel(debug_cg);

    if ( modules.size() == 0 )
        fatalError("no modules given to linker");

    std::list<string> module_names;
    llvm::Linker linker("<HILTI>", output, llvm::getGlobalContext());

    linker.addSystemPaths();

    for ( auto p : _paths ) {
        llvm::sys::Path path(p);
        linker.addPath(path);
    }

    for ( auto i : modules ) {

        auto name = i->getModuleIdentifier();

        if ( isHiltiModule(i) )
            module_names.push_back(name);

        string err;

        if ( linker.LinkInModule(i, &err) ) {
            error(linker, "module", name, err);
            return nullptr;
        }

        if ( verify && ! util::llvmVerifyModule(linker.getModule()) ) {
            error(linker, "verify", name);
            return nullptr;
        }
    }

    if ( module_names.size() ) {
        addModuleInfo(module_names, linker.getModule());
        addGlobalsInfo(module_names, linker.getModule());
        makeHooks(module_names, linker.getModule());
    }

    // Link in the libraries last as they need the autogenerated symbols.

    for ( auto i : _bcas ) {
        llvm::sys::Path path(i);
        bool native = false;
        if ( linker.LinkInArchive(path, native) ) {
            error(linker, "archive", i);
            return nullptr;
        }
    }

    for ( auto i : _libs ) {
        bool native = true;
        if ( linker.LinkInLibrary(i, native) ) {
            error(linker, "library", i);
            return nullptr;
        }
    }

    return linker.releaseModule();
}

bool Linker::isHiltiModule(llvm::Module* module)
{
    string id = module->getModuleIdentifier();
    string name = string(symbols::MetaModule) + "." + id;

    auto md = module->getNamedMetadata(name);
    bool is_hilti = (md != nullptr);

    if ( is_hilti )
        debug(1, ::util::fmt("module %s has HILTI meta data", id.c_str()));
    else
        debug(1, ::util::fmt("module %s does not have HILTI meta data", id.c_str()));

    return (md != nullptr);
}

void Linker::joinFunctions(const char* new_func, const char* meta, llvm::FunctionType* default_ftype, const std::list<string>& module_names, llvm::Module* module)
{
    auto md = module->getNamedMetadata(meta);

    IRBuilder* builder = nullptr;
    llvm::LLVMContext& ctx = llvm::getGlobalContext();
    llvm::Function* nfunc = nullptr;

    if ( ! md ) {
        // No such meta node, i.e., no module defines the function
        //
        // Create a function with the given type so that it exists, even if
        // empty. In this case, the function type doesn't need to have the
        // exact types as long as the cc does the right thing.
        nfunc = llvm::Function::Create(default_ftype, llvm::Function::ExternalLinkage, new_func, module);
        nfunc->setCallingConv(llvm::CallingConv::C);
        builder = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "", nfunc));
        builder->CreateRetVoid();
        return;
    }

    for ( int i = 0; i < md->getNumOperands(); ++i ) {
        auto node = llvm::cast<llvm::MDNode>(md->getOperand(i));
        auto func = llvm::cast<llvm::Function>(node->getOperand(0));

        if ( ! builder ) {
            // Create new function with same signature.
            auto ftype_ptr = llvm::cast<llvm::PointerType>(func->getType());
            auto ftype = llvm::cast<llvm::FunctionType>(ftype_ptr->getElementType());
            nfunc = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, new_func, module);
            nfunc->setCallingConv(llvm::CallingConv::C);
            builder = util::newBuilder(ctx, llvm::BasicBlock::Create(ctx, "", nfunc));
        }

        std::vector<llvm::Value*> params;
        for ( auto a = nfunc->arg_begin(); a != nfunc->arg_end(); ++a )
            params.push_back(a);

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

        decl.func = llvm::cast<llvm::Function>(node->getOperand(1));

        if ( node->getNumOperands() > 2 )
            decl.result = node->getOperand(1)->getType();
        else
            decl.result = nullptr;

        auto h = hooks.find(name);

        if ( h == hooks.end() )
            hooks.insert(std::make_pair(name, decl));

        else {
            // Already declared, make sure it matches.
            auto other = (*h).second;

            if ( decl.func != other.func || decl.result != other.result )
                fatalError(::util::fmt("inconsistent declarations found for hook %s", name.str().c_str()));
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

#ifdef DEBUG
    _debugDumpHooks(this, hooks);
#endif

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

void Linker::addModuleInfo(const std::list<string>& module_names, llvm::Module* module)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(ctx, 8), 0);
    std::vector<llvm::Type*> args = { voidp, voidp };
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), args, false);

    joinFunctions(symbols::FunctionModulesInit, symbols::MetaModuleInit, ftype, module_names, module);
}

void Linker::addGlobalsInfo(const std::list<string>& module_names, llvm::Module* module)
{
    llvm::LLVMContext& ctx = llvm::getGlobalContext();

    auto voidp = llvm::PointerType::get(llvm::IntegerType::get(ctx, 8), 0);
    std::vector<llvm::Type*> args = { voidp, voidp };
    auto ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), args, false);

    joinFunctions(symbols::FunctionGlobalsInit, symbols::MetaGlobalsInit, ftype, module_names, module);
    joinFunctions(symbols::FunctionGlobalsDtor, symbols::MetaGlobalsDtor, ftype, module_names, module);

    // Create the joint globals type and determine its size.

    std::vector<llvm::Type*> globals;
    GlobalsBasePass::transform_map tmap;

    for ( auto name : module_names ) {
        // Add module's globals to the joint type and keep track of the index in there.
        auto type = module->getTypeByName(symbols::TypeGlobals + string(".") + name);
        auto base_func = module->getFunction(symbols::FuncGlobalsBase + string(".") + name);

        if ( ! type )
            fatalError(::util::fmt("no type for globals defined in module %s", name.c_str()));

        if ( ! base_func ) {
            debug(1, ::util::fmt("no globals.base function defined for module %s", name.c_str()));
            continue;
        }

        if ( type && base_func ) {
            auto idx = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), globals.size());
            tmap.insert(std::make_pair(base_func, idx));
            globals.push_back(type);
        }
    }

    // Create the joint globals type.
    auto ty_globals = llvm::StructType::create(ctx, globals, symbols::TypeGlobals);

    // Create the global storing the size, using the "portable sizeof" idiom ...
    auto null = llvm::Constant::getNullValue(llvm::PointerType::get(ty_globals, 0));
    auto one = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 32), 1);
    auto addr = llvm::ConstantExpr::getGetElementPtr(null, one);
    auto size = llvm::ConstantExpr::getPtrToInt(addr, llvm::Type::getIntNTy(ctx, 64));
    auto glob = new llvm::GlobalVariable(*module, size->getType(), false, llvm::GlobalValue::ExternalLinkage, size, symbols::ConstantGlobalsSize);

    // Now run a pass over all the modules replacing the
    // @hlt.globals.base.<Module>() calls.
    auto pass = new GlobalsBasePass();
    pass->tmap = tmap;
    pass->globals_type = ty_globals;

    llvm::PassManager mgr;
    mgr.add(pass);
    mgr.run(*module);
}

