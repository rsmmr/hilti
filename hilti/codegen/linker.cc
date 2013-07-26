
#include "linker.h"
#include "util.h"
#include "codegen.h"
#include "../options.h"
#include "../context.h"

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
   llvm::Type* execution_context_type;

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

        auto i0 = llvm::CastInst::CreatePointerCast(ectx, execution_context_type);
        auto i1 = llvm::GetElementPtrInst::Create(i0, indices, "");
        auto i2 = llvm::CastInst::CreatePointerCast(i1, llvm::PointerType::get(globals_type, 0));

        llvm::BasicBlock& entry_block = func->getEntryBlock();
        llvm::BasicBlock::InstListType& instrs = entry_block.getInstList();

        instrs.push_front(i2);
        instrs.push_front(i1);
        instrs.push_front(i0);

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

    llvm::Module* composite = new llvm::Module("<HILTI>", llvm::getGlobalContext());
    llvm::Linker linker(composite);

    std::list<string> module_names;

    // Link in all the modules.

    for ( auto i : modules ) {

        auto name = i->getModuleIdentifier();

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
        // We need to add these to a separate module that we then link in; if
        // we added it to the module directly, names wouldn't be unified
        // correctly.
        auto linker_stuff = new ::llvm::Module("__linker_stuff", llvm::getGlobalContext());
        addModuleInfo(linker_stuff, module_names, linker.getModule());
        addGlobalsInfo(linker_stuff, module_names, linker.getModule());

        string err;

        linkInModule(&linker, linker_stuff);
        makeHooks(module_names, linker.getModule());
    }

    return linker.getModule();
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

    // Create the joint globals type and determine its size.

    std::vector<llvm::Type*> globals;
    GlobalsBasePass::transform_map tmap;

    for ( auto name : module_names ) {
        // Add module's globals to the joint type and keep track of the index in there.
        auto type = module->getTypeByName(symbols::TypeGlobals + string(".") + name);
        auto base_func = module->getFunction(symbols::FuncGlobalsBase + string(".") + name);

        if ( ! type )
            fatalError("no type for globals defined", name);

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

    // Now run a pass over all the modules replacing the
    // @hlt.globals.base.<Module>() calls.
    auto pass = new GlobalsBasePass();
    pass->tmap = tmap;
    pass->globals_type = ty_globals;

    auto md = Linker::moduleMeta(module, std::make_shared<hilti::ID>(module_names.front()));
    assert(md);
    pass->execution_context_type = md->getOperand(symbols::MetaModuleExecutionContextType)->getOperand(0)->getType();

    llvm::PassManager mgr;
    mgr.add(pass);
    mgr.run(*module);
}

///// This remaining code is here adapted from LLVM 3.2. It got removed from
///// LLVM 3.3, and there's probably a better way to do this, but we keep using
///// it for now.

//===- lib/Linker/LinkArchives.cpp - Link LLVM objects and libraries ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains routines to handle linking together LLVM bitcode files,
// and to handle annoying things like static libraries.
//
//===----------------------------------------------------------------------===//

#include <llvm/Linker.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SetOperations.h>
#include <llvm/Bitcode/Archive.h>
#include <memory>
#include <set>
using namespace llvm;

/// GetAllUndefinedSymbols - calculates the set of undefined symbols that still
/// exist in an LLVM module. This is a bit tricky because there may be two
/// symbols with the same name but different LLVM types that will be resolved to
/// each other but aren't currently (thus we need to treat it as resolved).
///
/// Inputs:
///  M - The module in which to find undefined symbols.
///
/// Outputs:
///  UndefinedSymbols - A set of C++ strings containing the name of all
///                     undefined symbols.
///
static void
GetAllUndefinedSymbols(llvm::Module *M, std::set<std::string> &UndefinedSymbols) {
  std::set<std::string> DefinedSymbols;
  UndefinedSymbols.clear();

  // If the program doesn't define a main, try pulling one in from a .a file.
  // This is needed for programs where the main function is defined in an
  // archive, such f2c'd programs.
  llvm::Function *Main = M->getFunction("main");
  if (Main == 0 || Main->isDeclaration())
    UndefinedSymbols.insert("main");

  for (llvm::Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportLinkage()
               && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName());
      }
    }

  for (llvm::Module::global_iterator I = M->global_begin(), E = M->global_end();
       I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportLinkage()
               && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName());
      }
    }

  for (llvm::Module::alias_iterator I = M->alias_begin(), E = M->alias_end();
       I != E; ++I)
    if (I->hasName())
      DefinedSymbols.insert(I->getName());

  // Prune out any defined symbols from the undefined symbols set...
  for (std::set<std::string>::iterator I = UndefinedSymbols.begin();
       I != UndefinedSymbols.end(); )
    if (DefinedSymbols.count(*I))
      UndefinedSymbols.erase(I++);  // This symbol really is defined!
    else
      ++I; // Keep this symbol in the undefined symbols list
}

void hilti::codegen::Linker::linkInModule(llvm::Linker* linker, llvm::Module* module)
{
    auto name = module->getModuleIdentifier();

    debug(1, ::util::fmt("linking in module %s", name));

    string err;

    if ( linker->linkInModule(module, &err) )
        fatalError("module linking error", name, err);
}

void hilti::codegen::Linker::linkInNativeLibrary(llvm::Linker* linker, const string& library)
{
    fatalError("native libraries not supported currently", library);

#if 0
    llvm::Module* Composite = linker->getModule();

    auto path = ::util::findInPaths(library, _paths);

    if ( path.empty() )
        fatalError("library not found", library);

    llvm::sys::Path Pathname(path);

    // If its an archive, try to link it in
    std::string Magic;
    Pathname.getMagicNumber(Magic, 64);

    fprintf(stderr, "MAIUGC: %s\n", Magic.c_str());

    switch ( llvm::sys::IdentifyFileType(Magic.c_str(), 64) ) {
     case sys::Archive_FileType:
        linkInArchive(linker, path);
        break;

     default:
        fatalError("unsupport library type", path, Magic);
    }
#endif
}

#if 0

/// LinkInArchive - opens an archive library and link in all objects which
/// provide symbols that are currently undefined.
///
/// Inputs:
///  Filename - The pathname of the archive.
///
/// Return Value:
///  TRUE  - An error occurred.
///  FALSE - No errors.
void hilti::codegen::Linker::linkInArchive(llvm::Linker* linker, const string& library)
{
    llvm::Module* Composite = linker->getModule();

    debug(1, ::util::fmt("linking in archive %s", library));

    auto path = ::util::findInPaths(library, _paths);

    if ( path.empty() )
        fatalError("archive not found", library);

    llvm::sys::Path Filename(path);

  // Make sure this is an archive file we're dealing with
  if (!Filename.isArchive())
      fatalError("not an archive", path);

  // Find all of the symbols currently undefined in the bitcode program.
  // If all the symbols are defined, the program is complete, and there is
  // no reason to link in any archive files.
  std::set<std::string> UndefinedSymbols;
  GetAllUndefinedSymbols(Composite, UndefinedSymbols);

  if (UndefinedSymbols.empty())
    return;  // No need to link anything in!

  std::string ErrMsg;
  std::auto_ptr<Archive> AutoArch (
    Archive::OpenAndLoadSymbols(Filename, llvm::getGlobalContext(), &ErrMsg));

  Archive* arch = AutoArch.get();

  if (!arch)
      fatalError("reading archive failed", Filename.str(), ErrMsg);

  if (!arch->isBitcodeArchive())
      fatalError("not a bitcode archive", Filename.str(), ErrMsg);

  // Save a set of symbols that are not defined by the archive. Since we're
  // entering a loop, there's no point searching for these multiple times. This
  // variable is used to "set_subtract" from the set of undefined symbols.
  std::set<std::string> NotDefinedByArchive;

  // Save the current set of undefined symbols, because we may have to make
  // multiple passes over the archive:
  std::set<std::string> CurrentlyUndefinedSymbols;

  do {
    CurrentlyUndefinedSymbols = UndefinedSymbols;

    // Find the modules we need to link into the target module.  Note that arch
    // keeps ownership of these modules and may return the same Module* from a
    // subsequent call.
    SmallVector<llvm::Module*, 16> Modules;
    if (!arch->findModulesDefiningSymbols(UndefinedSymbols, Modules, &ErrMsg))
        fatalError("finding symbols failed", Filename.str(), ErrMsg);

    // If we didn't find any more modules to link this time, we are done
    // searching this archive.
    if (Modules.empty())
      break;

    // Any symbols remaining in UndefinedSymbols after
    // findModulesDefiningSymbols are ones that the archive does not define. So
    // we add them to the NotDefinedByArchive variable now.
    NotDefinedByArchive.insert(UndefinedSymbols.begin(),
        UndefinedSymbols.end());

    // Loop over all the Modules that we got back from the archive
    for (SmallVectorImpl<llvm::Module*>::iterator I=Modules.begin(), E=Modules.end();
         I != E; ++I) {

      // Get the module we must link in.
      std::string moduleErrorMsg;
      llvm::Module* aModule = *I;
      if (aModule != NULL) {
        if (aModule->MaterializeAll(&moduleErrorMsg))
            fatalError("loading module failed", Filename.str(), moduleErrorMsg);
        }

        // Link it in
        linkInModule(linker, aModule);
    }

    // Get the undefined symbols from the aggregate module. This recomputes the
    // symbols we still need after the new modules have been linked in.
    GetAllUndefinedSymbols(Composite, UndefinedSymbols);

    // At this point we have two sets of undefined symbols: UndefinedSymbols
    // which holds the undefined symbols from all the modules, and
    // NotDefinedByArchive which holds symbols we know the archive doesn't
    // define. There's no point searching for symbols that we won't find in the
    // archive so we subtract these sets.
    set_subtract(UndefinedSymbols, NotDefinedByArchive);

    // If there's no symbols left, no point in continuing to search the
    // archive.
    if (UndefinedSymbols.empty())
      break;
  } while (CurrentlyUndefinedSymbols != UndefinedSymbols);
}

#endif
