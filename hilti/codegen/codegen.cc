
#include <util/util.h>

#include "../module.h"

#include "codegen.h"
#include "util.h"

#include "loader.h"
#include "storer.h"
#include "unpacker.h"
#include "field-builder.h"
#include "coercer.h"
#include "stmt-builder.h"
#include "type-builder.h"
#include "abi.h"
#include "debug-info-builder.h"
#include "passes/collector.h"
#include "builder/nodes.h"

#include "libhilti/enum.h"

using namespace hilti;
using namespace codegen;

CodeGen::CodeGen(const path_list& libdirs, int cg_debug_level)
    : _coercer(new Coercer(this)),
      _loader(new Loader(this)),
      _storer(new Storer(this)),
      _unpacker(new Unpacker(this)),
      _field_builder(new FieldBuilder(this)),
      _stmt_builder(new StatementBuilder(this)),
      _type_builder(new TypeBuilder(this)),
      _debug_info_builder(new DebugInfoBuilder(this)),
      _collector(new passes::Collector())
{
    _libdirs = libdirs;
    setLoggerName("codegen");
    debugSetLevel(cg_debug_level);
}

CodeGen::~CodeGen()
{}

llvm::Module* CodeGen::generateLLVM(shared_ptr<hilti::Module> hltmod, bool verify, int debug, int profile)
{
    _verify = verify;
    _debug_level = debug;
    _profile_level = profile;

    _hilti_module = hltmod;
    _functions.clear();

    if ( ! _collector->run(hltmod) )
        return nullptr;

    try {
        if ( ! _libhilti ) {
            string libhilti = ::util::findInPaths("libhilti.ll", _libdirs);

            if ( libhilti.size() == 0 )
                fatalError("cannot find libhilti.ll in library search path");

            llvm::SMDiagnostic diag;

            _libhilti = llvm::ParseAssemblyFile(libhilti, diag, llvmContext());

            if ( ! _libhilti )
                fatalError(string("cannot load libhilti.ll: ") + diag.getMessage() + "  (\"" + diag.getLineContents() + "\")");
        }

        _module = new ::llvm::Module(util::mangle(hltmod->id(), false), llvmContext());
        _abi = std::move(ABI::createABI(this));

        createInitFunction();

        initGlobals();

        // Kick-off recursive code generation.
        _stmt_builder->llvmStatement(hltmod->body());

        finishInitFunction();

        createGlobalsInitFunction();

        createLinkerData();

        createRtti();

        _type_builder->finalize();

        return _module;
    }

    catch ( const ast::FatalLoggerError& err ) {
        // Message has already been printed.
        return nullptr;
    }

}

void CodeGen::llvmInsertComment(const string& comment)
{
    _functions.back()->next_comment = comment;
}

llvm::Value* CodeGen::llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src, shared_ptr<hilti::Type> dst)
{
    return _coercer->llvmCoerceTo(value, src, dst);
}

llvm::Type* CodeGen::llvmLibType(const string& name)
{
    auto type = lookupCachedType("libhilti", name);

    if ( type )
        return type;

    type = _libhilti->getTypeByName(name);

    if ( ! type )
        internalError(::util::fmt("type %s not found in libhilti.ll", name.c_str()));

    // We need to recreate the type as otherwise the linker gets messed up
    // when we reuse the same library value directly (and in separate
    // modules).

    auto stype = llvm::cast<llvm::StructType>(type);

    std::vector<llvm::Type*> fields;
    for ( auto i = stype->element_begin(); i != stype->element_end(); ++i )
        fields.push_back(*i);

    type = llvm::StructType::create(llvmContext(), fields, string(name));
    return cacheType("libhilti", name, type);
}

llvm::Type* CodeGen::replaceLibType(llvm::Type* ntype)
{
    auto t = ntype;
    int depth = 0;

    while ( true ) {
        auto ptype = llvm::dyn_cast<llvm::PointerType>(t);
        if ( ! ptype )
            break;

        t = ptype->getElementType();
        ++depth;
    }

    auto stype = llvm::dyn_cast<llvm::StructType>(t);

    if ( stype ) {
        auto name = stype->getName().str();

        if ( name.size() ) {
            auto i = name.rfind(".");
            if ( i != string::npos && isdigit(name[i+1]) )
                name = name.substr(0, i);
        }

        if ( _libhilti->getTypeByName(name) ) {
            ntype = llvmLibType(name.c_str());

            while ( depth-- )
                ntype = llvm::PointerType::get(ntype, 0);
        }
    }

    return ntype;
}

llvm::Function* CodeGen::llvmLibFunction(const string& name)
{
    llvm::Value* val = lookupCachedValue("function", name);
    if ( val )
        return llvm::cast<llvm::Function>(val);

    auto func = _libhilti->getFunction(name);
    if ( ! func )
        internalError(::util::fmt("function %s not found in libhilti.ll", name.c_str()));

    // As we recreate the library types in llvmLibType, they now won't match
    // anymore what function prototype specify. So we need to recreate the
    // function as well. Sigh.

    std::vector<llvm::Type *> args;

    for ( auto arg = func->arg_begin(); arg != func->arg_end(); ++arg ) {
        args.push_back(replaceLibType(arg->getType()));
    }

    auto rtype = replaceLibType(func->getReturnType());
    auto ftype = llvm::FunctionType::get(rtype, args, false);
    auto nfunc = llvm::Function::Create(ftype, func->getLinkage(), func->getName(), _module);

    cacheValue("function", name, nfunc);
    return nfunc;
}

llvm::GlobalVariable* CodeGen::llvmLibGlobal(const string& name)
{
    auto glob = _libhilti->getGlobalVariable(name, true);
    if ( ! glob )
        internalError(::util::fmt("global %s not found in libhilti.ll", name.c_str()));

    return glob;
}

llvm::Value* CodeGen::llvmLocal(const string& name)
{
    auto map = _functions.back()->locals;

    auto i = map.find(name);

    if ( i == map.end() )
        internalError("unknown local " + name);

    return i->second.first;
}

llvm::Value* CodeGen::llvmGlobal(shared_ptr<Variable> var)
{
    return llvmGlobal(var.get());
}

llvm::Value* CodeGen::llvmGlobal(Variable* var)
{
    auto i = _globals.find(var);
    if ( i == _globals.end() )
        internalError(::util::fmt("undefined global %s", var->id()->name().c_str()));

    auto idx = i->second;
    auto base = llvmCreateCall(_globals_base_func);
    return llvmGEP(base, llvmGEPIdx(0), idx);
}


llvm::Value* CodeGen::llvmValue(shared_ptr<Expression> expr, shared_ptr<hilti::Type> coerce_to, bool cctor)
{
    return _loader->llvmValue(expr, cctor, coerce_to);
}

#if 0
llvm::Value* CodeGen::llvmValue(shared_ptr<Constant> constant)
{
    return _loader->llvmValue(constant);
}
#endif

llvm::Value* CodeGen::llvmExecutionContext()
{
    auto ctx = _functions.back()->context;

    if ( ctx )
        return ctx;

    for ( auto arg = function()->arg_begin(); arg != function()->arg_end(); ++arg ) {
        if ( arg->getName() == symbols::ArgExecutionContext )
            return arg;
    }

    internalError("no context argument found in llvmExecutionContext");
    return 0;
}

llvm::Value* CodeGen::llvmThreadMgr()
{
    return llvmCallC("hlt_global_thread_mgr", {}, false, false);
}

llvm::Value* CodeGen::llvmGlobalExecutionContext()
{
    return llvmCallC("hlt_global_execution_context", {}, false, false);
}

llvm::Constant* CodeGen::llvmSizeOf(llvm::Constant* v)
{
    return llvmSizeOf(v->getType());
}

llvm::Constant* CodeGen::llvmSizeOf(llvm::Type* t)
{
    // Computer size using the "portable sizeof" idiom ...
    auto null = llvmConstNull(llvmTypePtr(t));
    auto addr = llvm::ConstantExpr::getGetElementPtr(null, llvmGEPIdx(1));
    return llvm::ConstantExpr::getPtrToInt(addr, llvmTypeInt(64));
}

void CodeGen::llvmStore(shared_ptr<hilti::Expression> target, llvm::Value* value, bool dtor_first)
{
    _storer->llvmStore(target, value, true, dtor_first);
}

std::pair<llvm::Value*, llvm::Value*> CodeGen::llvmUnpack(
                   shared_ptr<Type> type, shared_ptr<Expression> begin,
                   shared_ptr<Expression> end, shared_ptr<Expression> fmt,
                   shared_ptr<Expression> arg,
                   const Location& location)
{
    UnpackArgs args;
    args.type = type;
    args.begin = begin ? llvmValue(begin) : nullptr;
    args.end = end ? llvmValue(end) : nullptr;
    args.fmt = fmt ? llvmValue(fmt) : nullptr;
    args.arg = arg ? llvmValue(arg) : nullptr;
    args.arg_type = arg ? arg->type() : nullptr;
    args.location = location;

    UnpackResult result = _unpacker->llvmUnpack(args);

    auto val = builder()->CreateLoad(result.value_ptr);
    auto iter = builder()->CreateLoad(result.iter_ptr);

    return std::make_pair(val, iter);
}

std::pair<llvm::Value*, llvm::Value*> CodeGen::llvmUnpack(
                   shared_ptr<Type> type, llvm::Value* begin,
                   llvm::Value* end, llvm::Value* fmt,
                   llvm::Value* arg, shared_ptr<Type> arg_type,
                   const Location& location)
{
    UnpackArgs args;
    args.type = type;
    args.begin = begin;
    args.end = end;
    args.fmt = fmt;
    args.arg = arg;
    args.arg_type = arg_type;
    args.location = location;

    UnpackResult result = _unpacker->llvmUnpack(args);

    auto val = builder()->CreateLoad(result.value_ptr);
    auto iter = builder()->CreateLoad(result.iter_ptr);

    return std::make_pair(val, iter);
}

llvm::Value* CodeGen::llvmParameter(shared_ptr<type::function::Parameter> param)
{
    auto name = param->id()->name();
    auto func = function();

    for ( auto arg = func->arg_begin(); arg != func->arg_end(); arg++ ) {
        if ( arg->getName() == name ) {
            if ( arg->hasByValAttr() )
                return builder()->CreateLoad(arg);
            else
                return arg;
        }
    }

    internalError("unknown parameter name " + name);
    return 0; // Never reached.
}

void CodeGen::llvmStore(statement::Instruction* instr, llvm::Value* value, bool dtor_first)
{
    _storer->llvmStore(instr->target(), value, true, dtor_first);
}

llvm::Function* CodeGen::pushFunction(llvm::Function* function, bool push_builder, bool abort_on_excpt, bool is_init_func)
{
    unique_ptr<FunctionState> state(new FunctionState);
    state->function = function;
    state->abort_on_excpt = abort_on_excpt;
    state->is_init_func = is_init_func;
    state->context = nullptr;
    _functions.push_back(std::move(state));

    if ( push_builder )
        pushBuilder("entry");

    return function;
}

void CodeGen::popFunction()
{
    if ( ! block()->getTerminator() )
        // Add a return if we don't have one yet.
        llvmReturn();

    llvmBuildExitBlock();
    llvmNormalizeBlocks();

    _functions.pop_back();
}

llvm::BasicBlock* CodeGen::block() const
{
    return builder()->GetInsertBlock();
}

bool CodeGen::functionEmpty() const
{
    auto func = function();
    auto size = function()->getBasicBlockList().size();

    if ( size == 0 || (size == 1 && func->getEntryBlock().empty()) )
        return true;

    return false;
}

IRBuilder* CodeGen::builder() const
{
    assert(_functions.size());
    return _functions.back()->builders.back();
}

IRBuilder* CodeGen::pushBuilder(IRBuilder* builder)
{
    assert(_functions.size());
    _functions.back()->builders.push_back(builder);
    return builder;
}

IRBuilder* CodeGen::pushBuilder(string name, bool reuse)
{
    return pushBuilder(newBuilder(name, reuse));
}

IRBuilder* CodeGen::newBuilder(string name, bool reuse, bool create)
{
    int cnt = 1;

    name = util::mangle(name, false);

    string n;

    while ( true ) {
        if ( cnt == 1 )
            n = ::util::fmt(".%s", name.c_str());
        else
            n = ::util::fmt(".%s.%d", name.c_str(), cnt);

        auto b = _functions.back()->builders_by_name.find(n);

        if ( b == _functions.back()->builders_by_name.end() )
            break;

        if ( reuse )
            return b->second;

        ++cnt;
    }

    if ( ! create )
    return 0;

    auto block = llvm::BasicBlock::Create(llvmContext(), n, function());
    auto builder = newBuilder(block);

    _functions.back()->builders_by_name.insert(std::make_pair(n, builder));

    return builder;
}

IRBuilder* CodeGen::newBuilder(llvm::BasicBlock* block, bool insert_at_beginning)
{
    return util::newBuilder(this, block, insert_at_beginning);
}

IRBuilder* CodeGen::builderForLabel(const string& name)
{
    return newBuilder(name, true);
}

IRBuilder* CodeGen::haveBuilder(const string& name)
{
    return newBuilder(name, true, false);
}

void CodeGen::popBuilder()
{
    assert(_functions.size());
    _functions.back()->builders.pop_back();
}

void CodeGen::createInitFunction()
{
    string name = util::mangle(_hilti_module->id(), true, nullptr, "init.module");

    CodeGen::parameter_list no_params;
    _module_init_func = llvmAddFunction(name, llvmTypeVoid(), no_params, false, type::function::HILTI_C);

    pushFunction(_module_init_func, true, false, true);
}

llvm::Function* CodeGen::llvmModuleInitFunction()
{
    return _module_init_func;
}

void CodeGen::finishInitFunction()
{
    // Make sure the function stack has been popped back to the original
    // state.
    assert(function() == _module_init_func);

    if ( ! functionEmpty() )
        // Add a terminator to the function.
        builder()->CreateRetVoid();

    else {
        // We haven't added anything to the function, just discard.
        _module_init_func->removeFromParent();
        _module_init_func = nullptr;
    }

    // Pop it.
    popFunction();
}

void CodeGen::initGlobals()
{
    // Create the %hlt.globals.type struct with all our global variables.
    std::vector<llvm::Type*> globals;

    for ( auto g : _collector->globals() ) {
        auto t = llvmType(g->type());
        _globals.insert(make_pair(g.get(), llvmGEPIdx(globals.size())));
        globals.push_back(t);
    }

    // This global will be accessed by our custom linker. Unfortunastely, it
    // seems, we can't store a type in a metadata node directly, which would
    // simplify the linker.
    string postfix = string(".") + _hilti_module->id()->name();
    _globals_type = llvmTypeStruct(symbols::TypeGlobals + postfix, globals);

    if ( globals.size() ) {
        // Create the @hlt.globals.base() function. This will be called when we
        // need the base address for our globals, but all calls will later be replaced by the linker.
        string name = symbols::FuncGlobalsBase + postfix;

        CodeGen::parameter_list no_params;
        _globals_base_func = llvmAddFunction(name, llvmTypePtr(_globals_type), no_params, false, type::function::C); // C to not mess with parameters.
    }
}

void CodeGen::createGlobalsInitFunction()
{
    // If we don't have any globals, we don't need any of the following.
    if ( _collector->globals().size() == 0 && _global_strings.size() == 0 )
        return;

    string postfix = string(".") + _hilti_module->id()->name();

    // Create a function that initializes our globals with defaults.
    auto name = util::mangle(_hilti_module->id(), true, nullptr, "init.globals");
    CodeGen::parameter_list no_params;
    _globals_init_func = llvmAddFunction(name, llvmTypeVoid(), no_params, false, type::function::HILTI_C);

    pushFunction(_globals_init_func, true, true, true);

#ifdef OLDSTRING
    // Init the global string constants.
    for ( auto gs : _global_strings ) {
        auto s = llvmStringFromData(gs.first);
        builder()->CreateStore(s, gs.second);
        finishStatement(); // Add cleanup.
    }
#endif

    // Init user defined globals.
    for ( auto g : _collector->globals() ) {
        auto init = g->init() ? llvmValue(g->init(), g->type(), true) : llvmInitVal(g->type());
        assert(init);
        auto addr = llvmGlobal(g.get());
        llvmCreateStore(init, addr);
    }

    if ( functionEmpty() ) {
        // We haven't added anything to the function, just discard.
        _globals_init_func->removeFromParent();
        _globals_init_func = nullptr;
    }

    popFunction();

    // Create a function that that destroys all the memory managed objects in
    // there.
    name = util::mangle(_hilti_module->id(), true, nullptr, "dtor.globals");
    _globals_dtor_func = llvmAddFunction(name, llvmTypeVoid(), no_params, false, type::function::HILTI_C);

    pushFunction(_globals_dtor_func, true, true);

    for ( auto g : _collector->globals() ) {
        auto val = llvmGlobal(g);
        llvmDtor(val, g->type(), true, "init-globals");
    }

    auto stype = shared_ptr<Type>(new type::String());

#ifdef OLDSTRING
    for ( auto gs : _global_strings ) {
        llvmDtor(gs.second, stype, true, "init-globals");
    }
#endif

    if ( functionEmpty() ) {
        // We haven't added anything to the function, just discard.
        _globals_dtor_func->removeFromParent();
        _globals_dtor_func = nullptr;
    }

    popFunction();
}

llvm::Value* CodeGen::llvmGlobalIndex(Variable* var)
{
    auto i = _globals.find(var);
    assert(i != _globals.end());
    return i->second;
}

void CodeGen::createLinkerData()
{
    // Add them main meta information node.
    string name = string(symbols::MetaModule) + "." + _module->getModuleIdentifier();

    auto old_md = _module->getNamedValue(name);

    if ( old_md )
        error("module meta data already exists");

    auto md  = _module->getOrInsertNamedMetadata(name);

    llvm::Value *version = llvm::ConstantInt::get(llvm::Type::getInt16Ty(llvmContext()), 1);
    llvm::Value *id = llvm::MDString::get(llvmContext(), _hilti_module->id()->name());
    llvm::Value *file = llvm::MDString::get(llvmContext(), _hilti_module->path());

    md->addOperand(util::llvmMdFromValue(llvmContext(), version));
    md->addOperand(util::llvmMdFromValue(llvmContext(), id));
    md->addOperand(util::llvmMdFromValue(llvmContext(), file));

    // Add the MD function arrays that the linker will merge.

    if ( _module_init_func ) {
        md  = _module->getOrInsertNamedMetadata(symbols::MetaModuleInit);
        md->addOperand(util::llvmMdFromValue(llvmContext(), _module_init_func));
    }

    if ( _globals_init_func ) {
        md  = _module->getOrInsertNamedMetadata(symbols::MetaGlobalsInit);
        md->addOperand(util::llvmMdFromValue(llvmContext(), _globals_init_func));
    }

    if ( _globals_dtor_func ) {
        md  = _module->getOrInsertNamedMetadata(symbols::MetaGlobalsDtor);
        md->addOperand(util::llvmMdFromValue(llvmContext(), _globals_dtor_func));
    }
}

void CodeGen::createRtti()
{
    for ( auto i : _hilti_module->exportedTypes() )
        llvmRtti(i);
}

llvm::Value* CodeGen::cacheValue(const string& component, const string& key, llvm::Value* val)
{
    string idx = component + "::" + key;
    _value_cache.insert(make_tuple(idx, val));
    return val;
}

llvm::Value* CodeGen::lookupCachedValue(const string& component, const string& key)
{
    string idx = component + "::" + key;
    auto i = _value_cache.find(idx);
    return (i != _value_cache.end()) ? i->second : nullptr;
}

llvm::Constant* CodeGen::cacheConstant(const string& component, const string& key, llvm::Constant* val)
{
    string idx = component + "::" + key;
    _constant_cache.insert(make_tuple(idx, val));
    return val;
}

llvm::Constant* CodeGen::lookupCachedConstant(const string& component, const string& key)
{
    string idx = component + "::" + key;
    auto i = _constant_cache.find(idx);
    return (i != _constant_cache.end()) ? i->second : nullptr;
}

llvm::Type* CodeGen::cacheType(const string& component, const string& key, llvm::Type* type)
{
    string idx = component + "::" + key;
    _type_cache.insert(make_tuple(idx, type));
    return type;
}

llvm::Type* CodeGen::lookupCachedType(const string& component, const string& key)
{
    string idx = component + "::" + key;
    auto i = _type_cache.find(idx);
    return (i != _type_cache.end()) ? i->second : nullptr;
}

codegen::TypeInfo* CodeGen::typeInfo(shared_ptr<hilti::Type> type)
{
    return _type_builder->typeInfo(type);
}

string CodeGen::uniqueName(const string& component, const string& str)
{
    string idx = component + "::" + str;

    auto i = _unique_names.find(idx);
    int cnt = (i != _unique_names.end()) ? i->second : 1;

    _unique_names.insert(make_tuple(idx, cnt + 1));

    if ( cnt == 1 )
        return ::util::fmt(".hlt.%s.%s", str.c_str(), _module->getModuleIdentifier().c_str());
    else
        return ::util::fmt(".hlt.%s.%s.%d", str.c_str(), _module->getModuleIdentifier().c_str(), cnt);

}

llvm::Type* CodeGen::llvmType(shared_ptr<hilti::Type> type)
{
    return _type_builder->llvmType(type);
}

llvm::Constant* CodeGen::llvmInitVal(shared_ptr<hilti::Type> type)
{
    llvm::Constant* val = lookupCachedConstant("type.init_val", type->render());

    if ( val )
        return val;

    auto ti = typeInfo(type);
    assert(ti->init_val);

    return cacheConstant("type.init_val", type->render(), ti->init_val);
}

llvm::GlobalVariable *CodeGen::llvmRttiPtr(shared_ptr<hilti::Type> type)
{
    llvm::Constant* val = lookupCachedConstant("type.rtti", type->render());

    if ( val ) {
        auto v = llvm::cast<llvm::GlobalVariable>(val);
        assert(v);
        return v;
    }

    // We add the global here first and cache it before we call llvmRtti() so
    // the recursive calls function properly.
    string name = util::mangle(string("hlt_type_info_hlt_") + type->render(), true, nullptr, "", false);
    name = ::util::strreplace(name, "_ref", "");
    name = ::util::strreplace(name, "_any", "");

    llvm::Constant* cval = llvmConstNull(llvmTypePtr(llvmTypeRtti()));
    auto constant1 = llvmAddConst(name, cval, true);
    constant1->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
    cacheConstant("type.rtti", type->render(), constant1);

    cval = _type_builder->llvmRtti(type);
    name = util::mangle(string("type_info_val_") + type->render(), true);
    auto constant2 = llvmAddConst(name, cval, true);
    constant2->setLinkage(llvm::GlobalValue::InternalLinkage);

    cval = llvm::ConstantExpr::getBitCast(constant2, llvmTypePtr(llvmTypeRtti()));
    constant1->setInitializer(llvm::ConstantExpr::getBitCast(constant2, llvmTypePtr(llvmTypeRtti())));

    return constant1;
}

llvm::Constant* CodeGen::llvmRtti(shared_ptr<hilti::Type> type)
{
    return llvmRttiPtr(type)->getInitializer();
}

llvm::Type* CodeGen::llvmTypeVoid() {
    return llvm::Type::getVoidTy(llvmContext());
}

llvm::Type* CodeGen::llvmTypeInt(int width) {
    return llvm::Type::getIntNTy(llvmContext(), width);
}

llvm::Type* CodeGen::llvmTypeDouble() {
    return llvm::Type::getDoubleTy(llvmContext());
}

llvm::Type* CodeGen::llvmTypeString() {
    return llvmTypePtr(llvmLibType("hlt.string"));
}

llvm::Type* CodeGen::llvmTypePtr(llvm::Type* t) {
    return llvm::PointerType::get(t ? t : llvmTypeInt(8), 0);
}

llvm::Type* CodeGen::llvmTypeExecutionContext() {
    return llvmLibType("hlt.execution_context");
}

llvm::Type* CodeGen::llvmTypeExceptionPtr() {
    return llvmTypePtr(llvmLibType("hlt.exception"));
}

llvm::Constant* CodeGen::llvmExceptionTypeObject(shared_ptr<type::Exception> excpt)
{
    auto libtype = excpt ? excpt->libraryType() : "hlt_exception_unspecified";

    if ( libtype.size() ) {
        // If it's a libhilti exception, create an extern declaration
        // pointing there.

        auto glob = lookupCachedConstant("type-exception-lib", libtype);

        if ( glob )
            return glob;

        auto g = llvmAddGlobal(libtype, llvmLibType("hlt.exception.type"), nullptr, true);
        g->setConstant(true);
        g->setInitializer(0);
        g->setLinkage(llvm::GlobalValue::ExternalLinkage);

        return cacheConstant("type-exception-lib", libtype, g);
    }

    else {
        // Otherwise, create the type (if we haven't already).

        assert(excpt->id());

        auto id = excpt->id()->pathAsString();
        auto glob = lookupCachedConstant("type-exception", id);

        if ( glob )
            return glob;

        auto name = llvmConstAsciizPtr(id);
        auto base = llvmExceptionTypeObject(ast::as<type::Exception>(excpt->baseType()));
        auto arg  = excpt->argType() ? llvmRttiPtr(excpt->argType()) : llvmConstNull(llvmTypePtr(llvmTypeRtti()));

        base = llvm::ConstantExpr::getBitCast(base, llvmTypePtr());
        arg = llvm::ConstantExpr::getBitCast(arg, llvmTypePtr(llvmTypePtr()));

        constant_list elems;
        elems.push_back(name);
        elems.push_back(base);
        elems.push_back(arg);
        auto val = llvmConstStruct(llvmLibType("hlt.exception.type"), elems);
        glob = llvmAddConst(::util::fmt("exception-%s", excpt->id()->name().c_str()), val);

        return cacheConstant("type-exception", id, glob);
    }
}

llvm::Type* CodeGen::llvmTypeRtti()
{
    return llvmLibType("hlt.type_info");
}

llvm::Type* CodeGen::llvmTypeStruct(const string& name, const std::vector<llvm::Type*>& fields, bool packed)
{
    if ( name.size() )
        return llvm::StructType::create(llvmContext(), fields, name, packed);
    else
        return llvm::StructType::get(llvmContext(), fields, packed);
}

llvm::ConstantInt* CodeGen::llvmConstInt(int64_t val, int64_t width)
{
    assert(width <= 64);
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(llvmContext(), width), val);
}

llvm::Constant* CodeGen::llvmConstDouble(double val)
{
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(llvmContext()), val);
}

llvm::Constant* CodeGen::llvmGEPIdx(int64_t idx)
{
    return llvmConstInt(idx, 32);
}

llvm::Constant* CodeGen::llvmConstNull(llvm::Type* t)
{
    if ( ! t )
        t = llvmTypePtr(llvmTypeInt(8)); // Will return "i8 *" aka "void *".

    return llvm::Constant::getNullValue(t);
}

llvm::Constant* CodeGen::llvmConstBytesEnd()
{
    auto t = llvmLibType("hlt.iterator.bytes");
    return llvmConstNull(t);
}

llvm::Constant* CodeGen::llvmConstArray(llvm::Type* t, const std::vector<llvm::Constant*>& elems)
{
    auto at = llvm::ArrayType::get(t, elems.size());
    return llvm::ConstantArray::get(at, elems);
}

llvm::Constant* CodeGen::llvmConstArray(const std::vector<llvm::Constant*>& elems)
{
    assert(elems.size() > 0);
    return llvmConstArray(elems.front()->getType(), elems);
}

llvm::Constant* CodeGen::llvmConstAsciiz(const string& str)
{
    auto c = lookupCachedConstant("const-asciiz", str);

    if ( c )
        return c;

    std::vector<llvm::Constant*> elems;

    for ( auto c : str )
        elems.push_back(llvmConstInt(c, 8));

    elems.push_back(llvmConstInt(0, 8));

    c = llvmConstArray(llvmTypeInt(8), elems);

    return cacheConstant("const-asciiz", str, c);
}

llvm::Constant* CodeGen::llvmConstAsciizPtr(const string& str)
{
    auto c = lookupCachedConstant("const-asciiz-ptr", str);

    if ( c )
        return c;

    auto cval = llvmConstAsciiz(str);
    auto ptr = llvmAddConst("asciiz", cval);

    c = llvm::ConstantExpr::getBitCast(ptr, llvmTypePtr());

    return cacheConstant("const-asciiz-ptr", str, c);

}

llvm::Constant* CodeGen::llvmConstStruct(const constant_list& elems, bool packed)
{
    if ( elems.size() )
        return llvm::ConstantStruct::getAnon(elems, packed);

    else {
        std::vector<llvm::Type*> empty;
        auto stype = llvmTypeStruct("", empty, packed);
        return llvmConstNull(stype);
    }
}

llvm::Constant* CodeGen::llvmConstStruct(llvm::Type* type, const constant_list& elems)
{
    return llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(type), elems);
}

llvm::Value* CodeGen::llvmEnum(const string& label)
{
    auto expr = _hilti_module->body()->scope()->lookup((std::make_shared<ID>(label)));

    if ( ! expr )
        throw ast::InternalError("llvmEnum: unknown enum label %s" + label);

    return llvmValue(expr);
}

llvm::Constant* CodeGen::llvmCastConst(llvm::Constant* c, llvm::Type* t)
{
    return llvm::ConstantExpr::getBitCast(c, t);
}

llvm::Value* CodeGen::llvmStringFromData(const string& str)
{
    std::vector<llvm::Constant*> vec_data;
    for ( auto c : str )
        vec_data.push_back(llvmConstInt(c, 8));

    auto array = llvmConstArray(llvmTypeInt(8), vec_data);
    llvm::Constant* data = llvmAddConst("string", array);
    data = llvm::ConstantExpr::getBitCast(data, llvmTypePtr());

    value_list args;
    args.push_back(data);
    args.push_back(llvmConstInt(str.size(), 64));
    return llvmCallC(llvmLibFunction("hlt_string_from_data"), args, true);
}

llvm::Value* CodeGen::llvmStringPtr(const string& s)
{
#ifdef OLDSTRING
    // If we're currently buildign the init function, we create the new value
    // directly and without caching, because otherwise we'd be telling the
    // init function to create, which it wouldn't do because it's already
    // running. Clear? :)

    if ( _functions.back()->is_init_func ) {
        auto data = llvmAddTmp("string", llvmTypeString(), llvmStringFromData(s));
        llvmDtorAfterInstruction(data, builder::string::type(), true);
        return data;
    }

    // While we can represent empty string as a nullptr, we also create an
    // object here in that case because we need to return a pointer.

    // See if we have already created an array for this string.
    llvm::Value* data = lookupCachedValue("llvmString", s);

    if ( ! data ) {
        data = llvmAddGlobal("string", llvmTypeString());
        cacheValue("llvmString", s, data);
        _global_strings.push_back(make_pair(s, data));
    }

    return data;
#endif

    auto val = llvmString(s);
    auto ptr = llvmAddTmp("string", val->getType(), val);
    return ptr;
}

llvm::Value* CodeGen::llvmString(const string& s)
{
    if ( s.size() == 0 )
        // The empty string is represented by a null pointer.
        return llvmConstNull(llvmTypeString());

    auto val = llvmStringFromData(s);
    llvmDtorAfterInstruction(val, builder::string::type(), false);
    return val;
}

llvm::Value* CodeGen::llvmValueStruct(const std::vector<llvm::Value*>& elems, bool packed)
{
    // This is quite a cumbersome way to create a struct on the fly but it
    // seems it's the best we can do when inserting non-const values.

    // Determine the final type.
    std::vector<llvm::Type*> types;
    for ( auto e : elems )
        types.push_back(e->getType());

    auto stype = llvmTypeStruct("", types, packed);
    llvm::Value* sval = llvmConstNull(stype);

    for ( int i = 0; i < elems.size(); ++i )
        sval = llvmInsertValue(sval, elems[i], i);

    return sval;
}

llvm::Value* CodeGen::llvmValueStruct(llvm::Type* stype, const std::vector<llvm::Value*>& elems, bool packed)
{
    // This is quite a cumbersome way to create a struct on the fly but it
    // seems it's the best we can do when inserting non-const values.

    llvm::Value* sval = llvmConstNull(stype);

    for ( int i = 0; i < elems.size(); ++i )
        sval = llvmInsertValue(sval, elems[i], i);

    return sval;
}

llvm::GlobalVariable* CodeGen::llvmAddConst(const string& name, llvm::Constant* val, bool use_name_as_is)
{
    auto mname = use_name_as_is ? name : uniqueName("const", name);
    auto glob = new llvm::GlobalVariable(*_module, val->getType(), true, llvm::GlobalValue::PrivateLinkage, val, mname);
    return glob;
}

llvm::GlobalVariable* CodeGen::llvmAddGlobal(const string& name, llvm::Type* type, llvm::Constant* init, bool use_name_as_is)
{
    auto mname = use_name_as_is ? name : uniqueName("global", name);

    if ( ! init )
        init = llvmConstNull(type);

    auto glob = new llvm::GlobalVariable(*_module, type, false, llvm::GlobalValue::PrivateLinkage, init, mname);
    return glob;
}

llvm::GlobalVariable* CodeGen::llvmAddGlobal(const string& name, llvm::Constant* init, bool use_name_as_is)
{
    assert(init);
    return llvmAddGlobal(name, init->getType(), init, use_name_as_is);
}

llvm::Value* CodeGen::llvmAddLocal(const string& name, shared_ptr<Type> type, llvm::Value* init)
{
    auto llvm_type = llvmType(type);

    bool init_in_entry_block = false;

    if ( ! init ) {
        init_in_entry_block = true;
        init = typeInfo(type)->init_val;
    }

    llvm::BasicBlock& block(function()->getEntryBlock());

    auto builder = newBuilder(&block, true);

    auto local = builder->CreateAlloca(llvm_type, 0, name);

    if ( init ) {
        if ( init_in_entry_block )
            pushBuilder(builder);

        llvmCreateStore(init, local);

        if ( init_in_entry_block )
            popBuilder();
    }

    _functions.back()->locals.insert(make_pair(name, std::make_pair(local, type)));

    delete builder;

    return local;
}

llvm::Value* CodeGen::llvmAddTmp(const string& name, llvm::Type* type, llvm::Value* init, bool reuse, int alignment)
{
    if ( ! init )
        init = llvmConstNull(type);

    string tname = "__tmp_" + name;

    if ( reuse ) {
        auto i = _functions.back()->tmps.find(tname);
        if ( i != _functions.back()->tmps.end() ) {
            auto tmp = i->second.first;
            llvmCreateStore(init, tmp);
            return tmp;
        }
    }

    llvm::BasicBlock& block(function()->getEntryBlock());

    auto tmp_builder = newBuilder(&block, true);
    auto tmp = tmp_builder->CreateAlloca(type, 0, tname);

    if ( alignment )
        tmp->setAlignment(alignment);

    if ( init )
        // Must be done in original block.
        llvmCreateStore(init, tmp);

    _functions.back()->tmps.insert(make_pair(tname, std::make_pair(tmp, nullptr)));

    delete tmp_builder;

    return tmp;
}

llvm::Value* CodeGen::llvmAddTmp(const string& name, llvm::Value* init, bool reuse)
{
    assert(init);
    return llvmAddTmp(name, init->getType(), init, reuse);
}

llvm::Function* CodeGen::llvmAddFunction(const string& name, llvm::Type* rtype, parameter_list params, bool internal, type::function::CallingConvention cc, bool skip_ctx)
{
    auto llvm_linkage = internal ? llvm::Function::InternalLinkage : llvm::Function::ExternalLinkage;
    auto llvm_cc = llvm::CallingConv::Fast;
    auto orig_rtype = rtype;

    // Determine the function's LLVM calling convention.
    switch ( cc ) {
     case type::function::HILTI:
     case type::function::HOOK:
        llvm_cc = llvm::CallingConv::Fast;
        break;

     case type::function::HILTI_C:
        llvm_cc = llvm::CallingConv::C;
        break;

     case type::function::C:
        llvm_cc = llvm::CallingConv::C;
        break;

     default:
        internalError("unknown calling convention in llvmAddFunction");
    }

    // See if we know that function already.
    auto func = _module->getFunction(name);

    if ( func ) {
        // Already created. But make sure cc and linkage are right.
        func->setCallingConv(llvm_cc);
        func->setLinkage(llvm_linkage);
        return func;
    }

    std::vector<std::pair<string, llvm::Type*>> llvm_args;

    // Adapt the return type according to calling convention.
    switch ( cc ) {
     case type::function::HILTI:
        break;

     case type::function::HOOK:
        // Hooks always return a boolean.
        rtype = llvmTypeInt(1);
        break;

     case type::function::HILTI_C:
        // TODO: Do ABI stuff.
        break;

     case type::function::C:
        // TODO: Do ABI stuff.
        break;

     default:
        internalError("unknown calling convention in llvmAddFunction");
    }

    // Adapt parameters according to calling conventions.
    for ( auto p : params ) {

        switch ( cc ) {
         case type::function::HILTI:
         case type::function::HOOK: {
             auto arg_llvm_type = llvmType(p.second);
             llvm_args.push_back(make_pair(p.first, arg_llvm_type));
             break;
         }

         case type::function::HILTI_C: {
             auto ptype = p.second;

             if ( ast::isA<hilti::type::TypeType>(ptype) )
                 // Pass just RTTI for type arguments.
                 llvm_args.push_back(make_tuple(string("ti_") + p.first, llvmTypePtr(llvmTypeRtti())));

             else {
                 TypeInfo* pti = typeInfo(ptype);
                 if ( pti->pass_type_info ) {
                     llvm_args.push_back(make_tuple(string("ti_") + p.first, llvmTypePtr(llvmTypeRtti())));
                     llvm_args.push_back(make_tuple(p.first, llvmTypePtr()));
                 }

                 else {
                     auto arg_llvm_type = llvmType(p.second);
                     llvm_args.push_back(make_tuple(p.first, arg_llvm_type));
                 }
             }

             break;
         }
         case type::function::C: {
             auto arg_llvm_type = llvmType(p.second);
             llvm_args.push_back(make_tuple(p.first, arg_llvm_type));
             break;
         }

     default:
            internalError("unknown calling convention in llvmAddFunction");
        }
    }

    // Add additional parameters our calling convention may need.
    switch ( cc ) {
     case type::function::HILTI:
        llvm_args.push_back(std::make_pair(symbols::ArgExecutionContext, llvmTypePtr(llvmTypeExecutionContext())));
        break;

     case type::function::HOOK:
        llvm_args.push_back(std::make_pair(symbols::ArgExecutionContext, llvmTypePtr(llvmTypeExecutionContext())));

        // Hooks with return value get an additional pointer to an instance
        // receiving it.
        if ( ! orig_rtype->isVoidTy() )
            llvm_args.push_back(std::make_pair("__rval", llvmTypePtr(orig_rtype)));
        break;

     case type::function::HILTI_C:
        llvm_args.push_back(std::make_pair(symbols::ArgException, llvmTypePtr(llvmTypeExceptionPtr())));

        if ( ! skip_ctx )
            llvm_args.push_back(std::make_pair(symbols::ArgExecutionContext, llvmTypePtr(llvmTypeExecutionContext())));
        break;

     case type::function::C:
        break;

     default:
        internalError("unknown calling convention in llvmAddFunction");
    }

    func = abi()->createFunction(name, rtype, llvm_args, llvm_linkage, _module, cc);
    func->setCallingConv(llvm_cc);

    return func;
}

llvm::Function* CodeGen::llvmAddFunction(const string& name, llvm::Type* rtype, llvm_parameter_list params, bool internal, bool force_name)
{
    auto mangled_name = force_name ? name : util::mangle(name, true, nullptr, "", false);

    auto llvm_linkage = internal ? llvm::Function::InternalLinkage : llvm::Function::ExternalLinkage;
    auto llvm_cc = llvm::CallingConv::C;

    std::vector<llvm::Type*> func_args;
    for ( auto a : params )
        func_args.push_back(a.second);

    auto ftype = llvm::FunctionType::get(rtype, func_args, false);
    auto func = llvm::Function::Create(ftype, llvm_linkage, mangled_name, _module);

    func->setCallingConv(llvm_cc);

    auto i = params.begin();
    for ( auto a = func->arg_begin(); a != func->arg_end(); ++a, ++i )
        a->setName(i->first);

    return func;
}

llvm::Function* CodeGen::llvmAddFunction(shared_ptr<Function> func, bool internal, type::function::CallingConvention cc, const string& force_name, bool skip_ctx)
{
    string use_name = force_name;

    if ( cc == type::function::DEFAULT )
        cc = func->type()->callingConvention();

    if ( cc == type::function::C )
        use_name = func->id()->name();

    parameter_list params;

    for ( auto p : func->type()->parameters() )
        params.push_back(make_pair(p->id()->name(), p->type()));

    auto name = use_name.size() ? use_name : util::mangle(func->id(), true, func->module()->id(), "", internal);

    auto rtype = llvmType(func->type()->result()->type());

    return llvmAddFunction(name, rtype, params, internal, cc, skip_ctx);
}

llvm::Function* CodeGen::llvmFunction(shared_ptr<Function> func, bool force_new)
{
    if ( func->type()->callingConvention() == type::function::C )
        // Don't mess with the name.
        return llvmAddFunction(func, false, type::function::C);

    bool internal = true;

    if ( func->module()->exported(func->id()) )
        internal = false;

    if ( func->type()->callingConvention() != type::function::HILTI )
        internal = false;

    string prefix;

    if ( ast::isA<Hook>(func) )
        prefix = ".hlt.hook";
    else if ( func->type()->callingConvention() == type::function::HILTI && ! internal )
        prefix = "hlt.";

    int cnt = 0;

    string name;

    while ( true ) {
        name = util::mangle(func->id(), true, func->module()->id(), prefix, internal);

        if ( ++cnt > 1 )
            name += ::util::fmt(".%d", cnt);

        auto llvm_func = _module->getFunction(name);

        if ( ! llvm_func )
            break;

        if ( ! force_new )
            return llvm_func;
    }

    return llvmAddFunction(func, internal, type::function::DEFAULT, name);
}

llvm::Function* CodeGen::llvmFunctionHookRun(shared_ptr<Hook> hook)
{
    auto cname = util::mangle(hook->id(), true, hook->module()->id(), "hook.run", true);
    auto fval = lookupCachedValue("function-hook", cname);

    if ( fval )
        return llvm::cast<llvm::Function>(fval);

    auto func = llvmAddFunction(hook, false, type::function::HOOK, cname);

    // Add meta information for the hook.
    std::vector<llvm::Value *> vals;

    // MD node for the hook's name.
    auto name = hook->id()->pathAsString();

    if ( ! hook->id()->isScoped() )
        name = _hilti_module->id()->name() + "::" + name;

    // Record the name.
    vals.push_back(llvm::MDString::get(llvmContext(), name));

    // Record the the function we want to call for running the hook.
    vals.push_back(func);

    // Returnd the return type, if we have one.
    auto rtype = hook->type()->result()->type();
    if ( ! ast::isA<type::Void>(rtype) )
        vals.push_back(llvmConstNull(llvmType(rtype)));

    // Create the global hook declaration node and add our vals as subnode in
    // there. The linker will merge all the top-level entries.
    auto md  = _module->getOrInsertNamedMetadata(symbols::MetaHookDecls);
    md->addOperand(llvm::MDNode::get(llvmContext(), vals));

    cacheValue("function-hook", cname, func);
    return func;
}

void CodeGen::llvmAddHookMetaData(shared_ptr<Hook> hook, llvm::Value *llvm_func)
{
    std::vector<llvm::Value *> vals;

    // Record the hook's name.
    auto name = hook->id()->pathAsString();

    if ( ! hook->id()->isScoped() )
        name = _hilti_module->id()->name() + "::" + name;

    vals.push_back(llvm::MDString::get(llvmContext(), name));

    // Record the function we want to have called when running the hook.
    vals.push_back(llvm_func);

    // Record priority and group.
    vals.push_back(llvmConstInt(hook->priority(), 64));
    vals.push_back(llvmConstInt(hook->group(), 64));

    // Create/get the global hook implementation node and add our vals as
    // subnode in there. The linker will merge all the top-level entries.
    auto md  = llvmModule()->getOrInsertNamedMetadata(symbols::MetaHookImpls);
    md->addOperand(llvm::MDNode::get(llvmContext(), vals));
}

llvm::Function* CodeGen::llvmFunction(shared_ptr<ID> id)
{
    auto expr = _hilti_module->body()->scope()->lookup(id);

    if ( ! expr )
        internalError(string("unknown function ") + id->name() + " in llvmFunction()");

    if ( ! ast::isA<expression::Function>(expr) )
        internalError(string("ID ") + id->name() + " is not a function in llvmFunction()");

    return llvmFunction(ast::as<expression::Function>(expr)->function());
}

llvm::Function* CodeGen::llvmFunction(const string& name)
{
    auto id = shared_ptr<ID>(new ID(name));
    return llvmFunction(id);
}

void CodeGen::llvmReturn(shared_ptr<Type> rtype, llvm::Value* result, bool result_cctored)
{
    if ( block()->getTerminator() )
        // Already terminated (and hopefully corrently).
        return;

    auto state = _functions.back().get();

    if ( ! state->exit_block )
        state->exit_block = llvm::BasicBlock::Create(llvmContext(), ".exit", function());

    if ( result ) {
        state->exits.push_back(std::make_pair(block(), result));

        if ( rtype && ! result_cctored )
            llvmCctor(result, rtype, false, "llvm-return");
    }

    builder()->CreateBr(state->exit_block);
}

void CodeGen::llvmNormalizeBlocks()
{
    auto func = _functions.back()->function;
    llvm::Function::BasicBlockListType& blocks = func->getBasicBlockList();

    std::list<llvm::BasicBlock *> to_remove;

    for ( auto b = blocks.begin(); b != blocks.end(); b++ ) {
        if ( b->empty() && llvm::pred_begin(b) == llvm::pred_end(b) )
            to_remove.push_back(b);
    }

    for ( auto b : to_remove )
        b->eraseFromParent();
}


void CodeGen::llvmBuildExitBlock()
{
    auto state = _functions.back().get();

    if ( ! state->exit_block )
        return;

    ++_in_build_exit;

    auto exit_builder = newBuilder(state->exit_block);

    llvm::PHINode* phi = nullptr;

    if ( state->exits.size() ) {
        auto rtype = state->exits.front().second->getType();
        phi = exit_builder->CreatePHI(rtype, state->exits.size());

        for ( auto e : state->exits )
            phi->addIncoming(e.second, e.first);
    }

    pushBuilder(exit_builder);

    llvmBuildFunctionCleanup();

    auto leave_func = _functions.back()->leave_func;

    if ( leave_func ) {

        auto name = ::util::fmt("%s::%s", _hilti_module->id()->name().c_str(), leave_func->id()->name().c_str());

        if ( debugLevel() > 0 ) {
            string msg = string("leaving ") + name;
            llvmDebugPrint("hilti-flow", msg);
        }

        if ( profileLevel() > 1 ) {
            // As this may be run in an exit block where we won't clean up
            // after us anymore, we do the string's mgt manually here.
            auto str = llvmStringFromData(string("func/") + name);
            llvmProfilerStop(str);
            llvmDtor(str, builder::string::type(), false, "exit-block/profiler-stop");
        }
    }

    if ( phi ) {
        if ( state->function->hasStructRetAttr() ) {
            // Need to store in argument.
            builder()->CreateStore(phi, state->function->arg_begin());
            builder()->CreateRetVoid();
        }

        else
            builder()->CreateRet(phi);
    }

    else
        builder()->CreateRetVoid();

    --_in_build_exit;
}

void CodeGen::llvmDtorAfterInstruction(llvm::Value* val, shared_ptr<Type> type, bool is_ptr)
{
    auto tmp = llvmAddTmp("dtor", val->getType());

    llvmCreateStore(val, tmp);

    _functions.back()->dtors_after_ins.push_back(std::make_tuple(std::make_tuple(tmp, is_ptr), type));
}

void CodeGen::llvmBuildFunctionCleanup()
{
    // Unref locals (incl. parameters).
    for ( auto l : _functions.back()->locals )
        llvmDtor(l.second.first, l.second.second, true, "func-cleanup-local");

    // Unref tmps.
    for ( auto unref : _functions.back()->dtors_after_ins ) {
        auto dtor = newBuilder("dtor-tmp");
        auto cont = newBuilder("cont");
        auto tmp = unref.first.first;
        auto val = builder()->CreateLoad(tmp);
        auto is_null = builder()->CreateIsNull(val);

        llvmCreateCondBr(is_null, cont, dtor);

        pushBuilder(dtor);

        llvm::Value* ptr_val = unref.first.second ? val : tmp;
        llvmDtor(ptr_val, unref.second, true, "function-cleanup-tmp");

        llvmCreateStore(llvmConstNull(val->getType()), tmp);
        llvmCreateBr(cont);
        popBuilder();

        pushBuilder(cont);

        // Leave on stack.
    }
}

llvm::Value* CodeGen::llvmGEP(llvm::Value* addr, llvm::Value* i1, llvm::Value* i2, llvm::Value* i3, llvm::Value* i4)
{
    std::vector<llvm::Value *> idx;

    if ( i1 )
        idx.push_back(i1);

    if ( i2 )
        idx.push_back(i2);

    if ( i3 )
        idx.push_back(i3);

    if ( i4 )
        idx.push_back(i4);

    return builder()->CreateGEP(addr, idx);
}

llvm::Constant* CodeGen::llvmGEP(llvm::Constant* addr, llvm::Value* i1, llvm::Value* i2, llvm::Value* i3, llvm::Value* i4)
{
    std::vector<llvm::Value *> idx;

    if ( i1 )
        idx.push_back(i1);

    if ( i2 )
        idx.push_back(i2);

    if ( i3 )
        idx.push_back(i3);

    if ( i4 )
        idx.push_back(i4);

    return llvm::ConstantExpr::getGetElementPtr(addr, idx);
}

llvm::CallInst* CodeGen::llvmCallC(llvm::Value* llvm_func, const value_list& args, bool add_hiltic_args, bool excpt_check)
{
    value_list call_args = args;

    llvm::Value* excpt = nullptr;

    if ( add_hiltic_args ) {
        excpt = llvmAddTmp("excpt", llvmTypeExceptionPtr(), 0, true);
        call_args.push_back(excpt);
        call_args.push_back(llvmExecutionContext());
    }

    auto result = llvmCreateCall(llvm_func, call_args);

    if ( excpt_check && excpt )
        llvmCheckCException(excpt);

    return result;
}

llvm::CallInst* CodeGen::llvmCallC(const string& llvm_func, const value_list& args, bool add_hiltic_args, bool excpt_check)
{
    auto f = llvmLibFunction(llvm_func);
    return llvmCallC(f, args, add_hiltic_args, excpt_check);
}

llvm::CallInst* CodeGen::llvmCallIntrinsic(llvm::Intrinsic::ID intr, std::vector<llvm::Type*> tys, const value_list& args)
{
    auto func = llvm::Intrinsic::getDeclaration(_module, intr, tys);
    assert(func);
    return llvmCreateCall(func, args);
}

void CodeGen::llvmRaiseException(const string& exception, shared_ptr<Node> node, llvm::Value* arg)
{
    return llvmRaiseException(exception, node->location(), arg);
}

llvm::Value* CodeGen::llvmExceptionNew(const string& exception, const Location& l, llvm::Value* arg)
{
    auto expr = _hilti_module->body()->scope()->lookup((std::make_shared<ID>(exception)));

    if ( ! expr )
        internalError(::util::fmt("unknown exception %s", exception.c_str()), l);

    auto type = ast::as<expression::Type>(expr)->typeValue();
    assert(type);

    auto etype = ast::as<type::Exception>(type);
    assert(etype);

    if ( arg )
        arg = builder()->CreateBitCast(arg, llvmTypePtr());

    CodeGen::value_list args;
    args.push_back(llvmExceptionTypeObject(etype));
    args.push_back(arg ? arg : llvmConstNull());
    args.push_back(llvmLocationString(l));

    return llvmCallC("hlt_exception_new", args, false, false);
}

llvm::Value* CodeGen::llvmExceptionArgument(llvm::Value* excpt)
{
    value_list args = { excpt };
    return llvmCallC("hlt_exception_arg", args, false, false);
}

llvm::Value* CodeGen::llvmExceptionFiber(llvm::Value* excpt)
{
    value_list args = { excpt };
    return llvmCallC("__hlt_exception_fiber", args, false, false);
}

void CodeGen::llvmRaiseException(const string& exception, const Location& l, llvm::Value* arg)
{
    auto excpt = llvmExceptionNew(exception, l, arg);
    llvmRaiseException(excpt, true);
}

void CodeGen::llvmRaiseException(llvm::Value* excpt, bool dtor)
{
    value_list args;
    args.push_back(llvmExecutionContext());
    args.push_back(excpt);
    llvmCallC("__hlt_context_set_exception", args, false, false);

    if ( dtor ) {
        auto ty = builder::reference::type(builder::exception::type(nullptr, nullptr));
        llvmDtor(excpt, ty, false, "raise-exception");
    }

    llvmTriggerExceptionHandling(true);
}

void CodeGen::llvmRethrowException()
{
    auto func = _functions.back()->function;

    auto rt = func->getReturnType();

    if ( rt->isVoidTy() && _functions.back()->function->hasStructRetAttr() )
        rt = func->arg_begin()->getType()->getPointerElementType();

    if ( rt->isVoidTy() )
        llvmReturn();
    else
        // This simply returns with a null value. The caller will check for a thrown exception.
        llvmReturn(0, llvmConstNull(rt));
}

void CodeGen::llvmClearException()
{
    value_list args;
    args.push_back(llvmExecutionContext());
    llvmCallC("__hlt_context_clear_exception", args, false, false);
}

llvm::Value* CodeGen::llvmCurrentException()
{
    value_list args;
    args.push_back(llvmExecutionContext());
    return llvmCallC("__hlt_context_get_exception", args, false, false);
}

llvm::Value* CodeGen::llvmCurrentFiber()
{
    value_list args;
    args.push_back(llvmExecutionContext());
    return llvmCallC("__hlt_context_get_fiber", args, false, false);
}

llvm::Value* CodeGen::llvmCurrentVID()
{
    value_list args;
    args.push_back(llvmExecutionContext());
    return llvmCallC("__hlt_context_get_vid", args, false, false);
}

llvm::Value* CodeGen::llvmCurrentThreadContext()
{
    if ( ! _hilti_module->context() )
        return nullptr;

    value_list args;
    args.push_back(llvmExecutionContext());
    auto ctx = llvmCallC("__hlt_context_get_thread_context", args, false, false);
    return builder()->CreateBitCast(ctx, llvmType(_hilti_module->context()));
}

void CodeGen::llvmSetCurrentThreadContext(shared_ptr<Type> type, llvm::Value* ctx)
{
    value_list args;
    args.push_back(llvmExecutionContext());
    args.push_back(llvmRtti(type));
    args.push_back(builder()->CreateBitCast(ctx, llvmTypePtr()));
    llvmCallC("__hlt_context_set_thread_context", args, false, false);
}

#if 0
llvm::Value* CodeGen::llvmSetCurrentFiber(llvm::Value* fiber)
{
    value_list args;
    args.push_back(llvmExecutionContext());
    args.push_back(fiber);
    return llvmCallC("__hlt_context_set_fiber", args, false, false);
}

llvm::Value* CodeGen::llvmClearCurrentFiber()
{
    value_list args;
    args.push_back(llvmExecutionContext());
    args.push_back(llvmConstNull(llvmTypePtr(llvmLibType("hlt.fiber"))));
    return llvmCallC("__hlt_context_set_fiber", args, false, false);
}

#endif


void CodeGen::llvmCheckCException(llvm::Value* excpt)
{
    if ( _in_build_exit )
        // Can't handle exeptions in exit block.
        return;

    auto eval = builder()->CreateLoad(excpt);
    auto is_null = builder()->CreateIsNull(eval);
    auto cont = newBuilder("no-excpt");
    auto raise = newBuilder("excpt-c");

    llvmCreateCondBr(is_null, cont, raise);

    pushBuilder(raise);
    llvmRaiseException(eval, true);
    popBuilder();

    pushBuilder(cont); // leave on stack.
}

void CodeGen::llvmCheckException()
{
    if ( _in_build_exit )
        // Can't handle exeptions in exit block.
        return;

    if ( ! _functions.back()->abort_on_excpt ) {
        llvmTriggerExceptionHandling(false);
        return;
    }

    // In the current function, an exception triggers an abort.

    auto excpt = llvmCurrentException();
    auto is_null = builder()->CreateIsNull(excpt);
    auto cont = newBuilder("no-excpt");
    auto abort = newBuilder("excpt-abort");

    llvmCreateCondBr(is_null, cont, abort);

    pushBuilder(abort);
    llvmCallC("__hlt_exception_print_uncaught_abort", { excpt, llvmExecutionContext() }, false, false);
    builder()->CreateUnreachable();
    popBuilder();

    pushBuilder(cont); // leave on stack.
}

llvm::Value* CodeGen::llvmMatchException(const string& name, llvm::Value* excpt)
{
    auto expr = _hilti_module->body()->scope()->lookup((std::make_shared<ID>(name)));

    if ( ! expr )
        internalError(::util::fmt("unknown exception %s", name.c_str()));

    auto type = ast::as<expression::Type>(expr)->typeValue();
    assert(type);

    auto etype = ast::as<type::Exception>(type);
    assert(etype);

    return llvmMatchException(etype, excpt);
}

llvm::Value* CodeGen::llvmMatchException(shared_ptr<type::Exception> etype, llvm::Value* excpt)
{
    CodeGen::value_list args = { excpt, llvmExceptionTypeObject(etype) };
    auto match = llvmCallC("__hlt_exception_match", args, false, false);
    return builder()->CreateICmpNE(match, llvmConstInt(0, 8));
}

void CodeGen::llvmTriggerExceptionHandling(bool known_exception)
{
    if ( _in_check_exception )
        // Make sure we don't recurse.
        return;

    ++_in_check_exception;

    llvm::Value* is_null = nullptr;
    IRBuilder* cont = 0;

    if ( ! known_exception ) {
        auto excpt = llvmCurrentException();
        is_null = builder()->CreateIsNull(excpt);
    }

    if ( _functions.back()->catches.size() ) {
        // At least one catch clause, check it.
        if ( ! known_exception ) {
            cont = newBuilder("no-excpt");
            llvmCreateCondBr(is_null, cont, _functions.back()->catches.back());
        }

        else {
            cont = newBuilder("cannot-be-reached");
            llvmCreateBr(_functions.back()->catches.back());
        }
    }

    else {
        auto unwind = newBuilder("unwind");

        if ( ! known_exception ) {
            cont = newBuilder("no-excpt");
            llvmCreateCondBr(is_null, cont, unwind);
        }

        else {
            cont = newBuilder("cannot-be-reached");
            llvmCreateBr(unwind);
        }

        pushBuilder(unwind);
        llvmRethrowException();
        popBuilder();
    }

    pushBuilder(cont);

    // Leave on stack.

    --_in_check_exception;
}


llvm::CallInst* CodeGen::llvmCreateCall(llvm::Value *callee, llvm::ArrayRef<llvm::Value *> args, const llvm::Twine &name)
{
    return util::checkedCreateCall(builder(), "CodeGen", callee, args, name);
}

llvm::CallInst* CodeGen::llvmCreateCall(llvm::Value *callee, const llvm::Twine &name)
{
    std::vector<llvm::Value*> no_params;
    return util::checkedCreateCall(builder(), "CodeGen", callee, no_params, name);
}

static void _dumpStore(llvm::Value *val, llvm::Value *ptr, const string& where, const string& msg)
{
    llvm::raw_os_ostream os(std::cerr);

    os << "\n";
    os << "=== LLVM store mismatch in " << where << ": " << msg << "\n";
    os << "\n";
    os << "-- Value type:\n";
    val->getType()->print(os);
    os << "\n";
    os << "-- Target type:\n";
    ptr->getType()->print(os);
    os << "\n";
    os.flush();

    ::util::abort_with_backtrace();
}

llvm::StoreInst* CodeGen::llvmCreateStore(llvm::Value *val, llvm::Value *ptr, bool isVolatile)
{
    auto ptype = ptr->getType();
    auto p = llvm::isa<llvm::PointerType>(ptype);

    if ( ! p )
        _dumpStore(val, ptr, "CodeGen", "target is not of pointer type");

    if ( llvm::cast<llvm::PointerType>(ptype)->getElementType() != val->getType() )
        _dumpStore(val, ptr, "CodeGen", "operand types do not match");

    return builder()->CreateStore(val, ptr, isVolatile);
}

llvm::BranchInst* CodeGen::llvmCreateBr(IRBuilder* b)
{
    return builder()->CreateBr(b->GetInsertBlock());
}

llvm::BranchInst* CodeGen::llvmCreateCondBr(llvm::Value* cond, IRBuilder* true_, IRBuilder* false_)
{
    return builder()->CreateCondBr(cond, true_->GetInsertBlock(), false_->GetInsertBlock());
}


llvm::Value* CodeGen::llvmInsertValue(llvm::Value* aggr, llvm::Value* val, unsigned int idx)
{
    if ( llvm::isa<llvm::VectorType>(aggr->getType()) )
        return builder()->CreateInsertElement(aggr, val, llvmConstInt(idx, 32));

    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return builder()->CreateInsertValue(aggr, val, vec);
}

llvm::Constant* CodeGen::llvmConstInsertValue(llvm::Constant* aggr, llvm::Constant* val, unsigned int idx)
{
    if ( llvm::isa<llvm::VectorType>(aggr->getType()) )
        return llvm::ConstantExpr::getInsertElement(aggr, val, llvmConstInt(idx, 32));

    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return llvm::ConstantExpr::getInsertValue(aggr, val, vec);
}

llvm::Value* CodeGen::llvmExtractValue(llvm::Value* aggr, unsigned int idx)
{
    if ( llvm::isa<llvm::VectorType>(aggr->getType()) )
        return builder()->CreateExtractElement(aggr, llvmConstInt(idx, 32));

    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return builder()->CreateExtractValue(aggr, vec);
}

llvm::Constant* CodeGen::llvmConstExtractValue(llvm::Constant* aggr, unsigned int idx)
{
    if ( llvm::isa<llvm::VectorType>(aggr->getType()) )
        return llvm::ConstantExpr::getExtractElement(aggr, llvmConstInt(idx, 32));

    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return llvm::ConstantExpr::getExtractValue(aggr, vec);
}

std::pair<llvm::Value*, llvm::Value*> CodeGen::llvmBuildCWrapper(shared_ptr<Function> func)
{
    // Name must match with ProtoGen::visit(declaration::Function* f).
    auto name = util::mangle(func->id(), true, func->module()->id(), "", false);

    auto rf1 = lookupCachedValue("c-wrappers", "entry-" + name);
    auto rf2 = lookupCachedValue("c-wrappers", "resume-" + name);

    if ( rf1 ) {
        assert(rf2);
        return std::make_pair(rf1, rf2);
    }

    auto ftype = func->type();
    auto rtype = ftype->result()->type();

    if ( ! func->body() ) {
        // No implementation, nothing to do here.
        internalError("llvmBuildCWrapper: not implemented for function without body; should return prototypes.");
        return std::make_pair((llvm::Value*)nullptr, (llvm::Value*)nullptr);
    }

    assert(ftype->callingConvention() == type::function::HILTI);
    assert(func->body());

    // Build the entry function.

    auto llvm_func = llvmAddFunction(func, false, type::function::HILTI_C, name, true);
    rf1 = llvm_func;

    auto args = llvm_func->arg_begin();

    pushFunction(llvm_func);
    _functions.back()->context = llvmGlobalExecutionContext();

    llvmClearException();

    expr_list params;

    auto arg = llvm_func->arg_begin();
    for ( auto a : ftype->parameters() )
        params.push_back(builder::codegen::create(a->type(), arg++));

    auto result = llvmCallInNewFiber(llvmFunction(func), ftype, params);

    // Copy exception over.
    auto ctx_excpt = llvmCurrentException();
    auto last_arg = llvm_func->arg_end();
    llvmGCAssign(--last_arg, ctx_excpt, builder::reference::type(builder::exception::typeAny()), false);

    if ( rtype->equal(shared_ptr<Type>(new type::Void())) )
        llvmReturn();
    else
        llvmReturn(rtype, result, true);

    popFunction();

    // Build the resume function.

        // Name must match with ProtoGen::visit(declaration::Function* f).
    name = util::mangle(func->id()->name() + "_resume", true, func->module()->id(), "", false);

    parameter_list fparams = { std::make_pair("yield_excpt", builder::reference::type(builder::exception::typeAny())) };
    llvm_func = llvmAddFunction(name, llvm_func->getReturnType(), fparams, false, type::function::HILTI_C, true);
    rf2 = llvm_func;

    pushFunction(llvm_func);
    _functions.back()->context = llvmGlobalExecutionContext();

    llvmClearException();

    auto yield_excpt = llvm_func->arg_begin();
    auto fiber = llvmExceptionFiber(yield_excpt);
    fiber = builder()->CreateBitCast(fiber, llvmTypePtr(llvmLibType("hlt.fiber")));

    value_list eargs = { yield_excpt };
    llvmCallC("__hlt_exception_clear_fiber", eargs, false, false);

    llvmDtor(yield_excpt, builder::reference::type(builder::exception::typeAny()), false, "c-wrapper/resume");

    result = llvmFiberStart(fiber, rtype);

    // Copy exception over.
    ctx_excpt = llvmCurrentException();
    llvmGCAssign(++llvm_func->arg_begin(), ctx_excpt, builder::reference::type(builder::exception::typeAny()), false, false);

    if ( rtype->equal(shared_ptr<Type>(new type::Void())) )
        llvmReturn();
    else
        llvmReturn(rtype, result, true);

    popFunction();

    cacheValue("c-wrappers", "entry-" + name, rf1);
    cacheValue("c-wrappers", "resume-" + name, rf2);

    return std::make_pair(rf1, rf2);
}

llvm::Value* CodeGen::llvmCall(llvm::Value* llvm_func, shared_ptr<type::Function> ftype, const expr_list args, bool excpt_check)
{
    auto result = llvmDoCall(llvm_func, nullptr, ftype, args, nullptr, excpt_check);
    return result;
}


llvm::Value* CodeGen::llvmCallInNewFiber(llvm::Value* llvm_func, shared_ptr<type::Function> ftype, const expr_list args)
{
    auto rtype = ftype->result()->type();

    // Create a struct value with all the arguments, plus the current context.

    CodeGen::type_list stypes;

    for ( auto a : args )
        stypes.push_back(llvmType(a->type()));

    auto sty = llvm::cast<llvm::StructType>(llvmTypeStruct("", stypes));
    llvm::Value* sval = llvmConstNull(sty);

    for ( int i = 0; i < args.size(); ++i ) {
        auto val = llvmValue(args[i]);
        sval = llvmInsertValue(sval, val, i);
    }

    // Create a function that receives the parameter struct and then calls the actual function.
    auto name = llvm_func->getName().str();

    llvm::Function* func = nullptr;

    llvm::Value* f = lookupCachedValue("fiber-func", name);

    if ( f )
        func = llvm::cast<llvm::Function>(f);

    if ( ! func) {
        llvm_parameter_list params = {
            std::make_pair("fiber", llvmTypePtr(llvmLibType("hlt.fiber"))),
            std::make_pair("fiber.args", llvmTypePtr(sty))
        };

        func = llvmAddFunction(string(".fiber.run") + name, llvmTypeVoid(), params, true, type::function::C);

        pushFunction(func);

        auto fiber = func->arg_begin();
        auto fsval = builder()->CreateLoad(++func->arg_begin());

        value_list cargs { fiber };
        _functions.back()->context = llvmCallC("hlt_fiber_context", cargs, false, false);

        expr_list fargs;

        for ( int i = 0; i < args.size(); ++i ) {
            auto val = llvmExtractValue(fsval, i);
            fargs.push_back(builder::codegen::create(args[i]->type(), val));
        }

        auto result = llvmCall(llvm_func, ftype, fargs, false);

        _functions.back()->context = nullptr;

        if ( ! rtype->equal(builder::void_::type()) ) {
            value_list args = { fiber };
            llvm::Value* rptr = llvmCallC("hlt_fiber_get_result_ptr", args, false, false);
            rptr = builder()->CreateBitCast(rptr, llvmTypePtr(llvmType(rtype)));
            llvmCreateStore(result, rptr);
        }

        builder()->CreateRetVoid();

        popFunction();

        cacheValue("fiber-func", name, func);
    }

    // Create the fiber and start it.

    auto tmp = llvmAddTmp("fiber.arg", sty, sval, true);
    auto funcp = builder()->CreateBitCast(func, llvmTypePtr());
    auto svalp = builder()->CreateBitCast(tmp, llvmTypePtr());

    value_list cargs { funcp, llvmExecutionContext(), svalp };
    auto fiber = llvmCallC("hlt_fiber_create", cargs, false, false);

    return llvmFiberStart(fiber, rtype);
}

llvm::Value* CodeGen::llvmFiberStart(llvm::Value* fiber, shared_ptr<Type> rtype)
{
    llvm::Value* rptr = 0;

    if ( ! rtype->equal(builder::void_::type()) ) {
        rptr = llvmAddTmp("fiber.result", llvmType(rtype), nullptr, true);
        auto rptr_casted = builder()->CreateBitCast(rptr, llvmTypePtr());
        llvmCallC("hlt_fiber_set_result_ptr", { fiber, rptr_casted }, false, false);
    }

    value_list cargs { fiber };
    auto result = llvmCallC("hlt_fiber_start", { fiber }, false, false);

    auto is_null = builder()->CreateIsNull(result);
    auto done = newBuilder("done");
    auto yield = newBuilder("yielded");

    llvmCreateCondBr(is_null, yield, done);

    pushBuilder(yield);

    CodeGen::value_list args = { fiber, llvmLocationString(Location::None) };
    auto excpt = llvmCallC("hlt_exception_new_yield", args, false, false);
    value_list eargs = { llvmExecutionContext(), excpt };
    llvmCallC("__hlt_context_set_exception", eargs, false, false);
    llvmDtor(excpt, builder::reference::type(builder::exception::typeAny()), false, "llvmFiberStart");
    llvmCreateBr(done);
    popBuilder();

    pushBuilder(done);

    if ( rptr )
        return builder()->CreateLoad(rptr);

    else
        return nullptr;

    // Leave builder on stack.
}

void CodeGen::llvmFiberYield(llvm::Value* fiber, shared_ptr<Type> blockable_ty, llvm::Value* blockable_val)
{
    if ( blockable_ty ) {
        assert(typeInfo(blockable_ty)->blockable.size());

        auto objptr = llvmAddTmp("obj", blockable_val->getType(), blockable_val, true);
        CodeGen::value_list args = { llvmRtti(blockable_ty), builder()->CreateBitCast(objptr, llvmTypePtr()) } ;
        blockable_val = llvmCallC("__hlt_object_blockable", args, true, true);
    }
    else
        blockable_val = llvmConstNull(llvmTypePtr(llvmLibType("hlt.blockable")));

    CodeGen::value_list args1 = { llvmExecutionContext(), blockable_val };
    llvmCallC("__hlt_context_set_blockable", args1, false, false);

    CodeGen::value_list args2 = { fiber };
    llvmCallC("hlt_fiber_yield", args2, false, false);
}

llvm::Value* CodeGen::llvmCallableBind(llvm::Value* llvm_func_val, shared_ptr<type::Function> ftype, const expr_list args, bool excpt_check)
{
    auto llvm_func = llvm::cast<llvm::Function>(llvm_func_val);
    auto rtype = ftype->result()->type();
    auto callable_type = std::make_shared<type::Callable>(rtype);

    // Create the struct. It starts like any callable and then comes our
    // additional stuff.
    auto cty = llvm::cast<llvm::StructType>(llvmLibType("hlt.callable"));

    auto arg_start = cty->getNumElements();

    CodeGen::type_list stypes;

    for ( int i = 0; i < cty->getNumElements(); ++i )
        stypes.push_back(cty->getElementType(i));

    for ( auto a : args )
        stypes.push_back(llvmType(a->type()));

    auto name = llvm_func->getName().str();
    auto sty = llvm::cast<llvm::StructType>(llvmTypeStruct(::string(".callable.args") + name, stypes));

    // Now fill a new callable object with its values.
    llvm::Value* c = llvmObjectNew(callable_type, sty);
    llvm::Value* s = builder()->CreateLoad(c);
    auto func_val = llvmCallableMakeFuncs(llvm_func, ftype, excpt_check, sty, name);
    func_val = builder()->CreateBitCast(func_val, stypes[1]); // FIXME: Not sure why we need this cast.
    s = llvmInsertValue(s, func_val, 1);

    for ( int i = 0; i < args.size(); ++i ) {
        auto val = llvmValue(args[i]);
        s = llvmInsertValue(s, val, arg_start + i);
        llvmCctor(val, args[i]->type(), false, "callable.call");
    }

    llvmCreateStore(s, c);
    return builder()->CreateBitCast(c, llvmTypePtr(cty));
}

llvm::Value* CodeGen::llvmCallableMakeFuncs(llvm::Function* llvm_func, shared_ptr<type::Function> ftype, bool excpt_check, llvm::StructType* sty, const string& name)
{
    llvm::Value* cached = lookupCachedValue("callable-func", name);

    if ( cached )
        return cached;

    auto rtype = ftype->result()->type();
    auto cty = llvm::cast<llvm::StructType>(llvmLibType("hlt.callable"));
    auto arg_start = cty->getNumElements();

    // Build the internal function that will later call the target.
    CodeGen::parameter_list params = { std::make_pair("callable", builder::reference::type(builder::callable::type(rtype))) };
    auto llvm_call_func = llvmAddFunction(string(".callable.run") + name, llvm_func->getReturnType(), params, true, type::function::HILTI);

    pushFunction(llvm_call_func);

    auto callable = builder()->CreateBitCast(llvm_call_func->arg_begin(), llvmTypePtr(sty));
    callable = builder()->CreateLoad(callable);

    expr_list nargs;

    int i = 0;
    for ( auto a : ftype->parameters() ) {
        auto val = llvmExtractValue(callable, arg_start + i);
        nargs.push_back(builder::codegen::create(a->type(), val));
        ++i;
    }

    auto result = llvmCall(llvm_func, ftype, nargs, excpt_check);

    // Don't call llvmReturn() here as it would create the normal function
    // return code and reref the result, which return.result will have
    // already done.
    if ( rtype->equal(builder::void_::type()) )
        builder()->CreateRetVoid();
    else
        builder()->CreateRet(result);

    popFunction();

    // If we have a return value, do variant that will discard it. This is for calling from C.
    llvm::Constant* llvm_call_func_no_return = nullptr;

    if ( rtype->equal(builder::void_::type()) )
        llvm_call_func_no_return = llvm_call_func;

    else {
        auto func_no_return = llvmAddFunction(string(".callable.run.no.result") + name, llvmTypeVoid(), params, true, type::function::HILTI);

        pushFunction(func_no_return);

        std::vector<llvm::Value*> fargs;
        for ( auto a = func_no_return->arg_begin(); a != func_no_return->arg_end(); ++a )
            fargs.push_back(a);

        llvmCreateCall(llvm_call_func, fargs);
        llvmReturn();
        popFunction();

        llvm_call_func_no_return = func_no_return;
    }

    // Build the internal function that will later dtor all the arguments in the struct.

    llvm::Constant* llvm_dtor_func = nullptr;

    if ( ftype->parameters().size() ) {
        CodeGen::llvm_parameter_list lparams = { std::make_pair("callable", llvmTypePtr(cty)) };
        auto dtor = llvmAddFunction(string(".callable.dtor") + name, llvmTypeVoid(), lparams, true, true);

        pushFunction(dtor);

        callable = llvm_call_func->arg_begin();
        callable = builder()->CreateBitCast(dtor->arg_begin(), llvmTypePtr(sty));
        callable = builder()->CreateLoad(callable);

        int i = 0;
        for ( auto a : ftype->parameters() ) {
            auto val = llvmExtractValue(callable, arg_start + i);
            llvmDtor(val, a->type(), false, "callable.run.dtor");
            ++i;
        }

        llvmReturn();
        popFunction();

        llvm_dtor_func = llvm::ConstantExpr::getBitCast(dtor, llvmTypePtr());
    }
    else
        llvm_dtor_func = llvmConstNull(llvmTypePtr());

    // Build the per-function object for this callable.

    auto ctyfunc = llvm::cast<llvm::StructType>(llvmLibType("hlt.callable.func"));
    auto ctyfuncval = llvmConstNull(ctyfunc);
    ctyfuncval = llvmConstInsertValue(ctyfuncval, llvm::ConstantExpr::getBitCast(llvm_call_func, llvmTypePtr()), 0);
    ctyfuncval = llvmConstInsertValue(ctyfuncval, llvm::ConstantExpr::getBitCast(llvm_call_func_no_return, llvmTypePtr()), 1);
    ctyfuncval = llvmConstInsertValue(ctyfuncval, llvm_dtor_func, 2);

    auto ctyfuncglob = llvmAddConst("callable.func" + name, ctyfuncval);

    return cacheValue("callable-func", name, ctyfuncglob);
}

llvm::Value* CodeGen::llvmCallableRun(shared_ptr<type::Callable> cty, llvm::Value* callable)
{
    value_list args = { callable, llvmExecutionContext() };

    std::vector<llvm::Type*> types;

    for ( auto a : args )
        types.push_back(a->getType());

    auto ftype = llvm::FunctionType::get(llvmType(cty->argType()), types, false);
    auto funcobj = llvmExtractValue(builder()->CreateLoad(callable), 1);
    funcobj = builder()->CreateBitCast(funcobj, llvmTypePtr(llvmLibType("hlt.callable.func")));  // FIXME: Not sure why we need this cast.
    auto func = llvmExtractValue(builder()->CreateLoad(funcobj), 0);
    func = builder()->CreateBitCast(func, llvmTypePtr(ftype));

    // Can't use the safer llvmCreateCall() here because we have casted a
    // generic pointer into oru function pointer.
    auto result = builder()->CreateCall(func, args);

    llvmCheckException();

    if ( cty->argType()->equal(shared_ptr<Type>(new type::Void())) )
        return nullptr;

    return result;
}

llvm::Value* CodeGen::llvmRunHook(shared_ptr<Hook> hook, const expr_list& args, llvm::Value* result)
{
    return llvmDoCall(nullptr, hook, hook->type(), args, result, true);
}

llvm::Value* CodeGen::llvmDoCall(llvm::Value* llvm_func, shared_ptr<Hook> hook, shared_ptr<type::Function> ftype, const expr_list& args, llvm::Value* hook_result, bool excpt_check)
{
    std::vector<llvm::Value*> llvm_args;

    // Prepare return value according to calling convention.

    switch ( ftype->callingConvention() ) {
     case type::function::HILTI:
     case type::function::HOOK:
        // Can pass directly.
        break;

     case type::function::HILTI_C:
        // FIXME: Do ABI stuff!
        break;

     case type::function::C:
        // Don't mess with arguments.
        break;

     default:
        internalError("unknown calling convention in llvmCall");
    }

    // Prepare parameters according to calling convention.

    auto arg = args.begin();

    for ( auto p : ftype->parameters() ) {
        auto ptype = p->type();

        auto coerced = (*arg)->coerceTo(ptype);
        auto arg_type = coerced->type();

        switch ( ftype->callingConvention() ) {
         case type::function::HILTI:
         case type::function::HOOK: {
            assert(! ast::isA<hilti::type::TypeType>(arg_type)); // Not supported.

            // Can pass directly but need context.
            auto arg_value = llvmValue(coerced);
            llvm_args.push_back(arg_value);
            break;
         }

         case type::function::HILTI_C: {
             if ( ast::isA<hilti::type::TypeType>(arg_type) ) {
                 // Pass just RTTI for type arguments.
                 auto tty = ast::as<hilti::type::TypeType>((*arg)->type());
                 assert(tty);
                 auto rtti = llvmRtti(tty->typeType());
                 llvm_args.push_back(rtti);
                 break;
             }

             auto arg_value = llvmValue(coerced);

             if ( typeInfo(ptype)->pass_type_info ) {
                 auto rtti = llvmRtti(arg_type);
                 auto arg_llvm_type = llvmType(arg_type);

                 llvm_args.push_back(rtti);
                 auto tmp = llvmAddTmp("arg", arg_llvm_type, arg_value);
                 llvm_args.push_back(builder()->CreateBitCast(tmp, llvmTypePtr()));
                 break;
             }

             llvm_args.push_back(arg_value);

             break;
         }

         case type::function::C: {
            assert(! ast::isA<hilti::type::TypeType>(arg_type)); // Not supported.

            // Don't mess with arguments.
            auto arg_value = llvmValue(coerced);
            llvm_args.push_back(arg_value);
            break;
         }

         default:
            internalError("unknown calling convention in llvmCall");
        }

        ++arg;
    }

    // Add additional parameters our calling convention may need.

    llvm::Value* excpt = nullptr;
    auto cc = ftype->callingConvention();

    switch ( cc ) {
     case type::function::HILTI:
        llvm_args.push_back(llvmExecutionContext());
        break;

     case type::function::HOOK:
        llvm_args.push_back(llvmExecutionContext());

        if ( hook_result )
            llvm_args.push_back(hook_result);
        break;

     case type::function::HILTI_C: {
         excpt = llvmAddTmp("excpt", llvmTypeExceptionPtr(), 0, true);
         llvm_args.push_back(excpt);
         llvm_args.push_back(llvmExecutionContext());
        break;
     }

     case type::function::C:
        break;

     default:
        internalError("unknown calling convention in llvmCall");
    }

    // If it's a hook, redirect the call to the function that the linker will
    // create.
    if ( hook )
        llvm_func = llvmFunctionHookRun(hook);

    // Apply calling convention.
    auto orig_args = llvm_args;
    auto func = llvm::cast<llvm::Function>(llvm_func);
    auto rtype = func->getReturnType();

    auto result = abi()->createCall(func, llvm_args, cc);

    switch ( cc ) {
     case type::function::HILTI_C: {
         if ( excpt_check )
             llvmCheckCException(excpt);

         else {
             value_list args;
             args.push_back(llvmExecutionContext());
             args.push_back(builder()->CreateLoad(excpt));
             llvmCallC("__hlt_context_set_exception", args, false, false);

             auto ty = builder::reference::type(builder::exception::typeAny());
             llvmDtor(excpt, ty, true, "llvmDoCall/excpt");
         }

         break;
     }

     default:
        if ( excpt_check )
            llvmCheckException();
        break;
        }
    return result;
}

llvm::Value* CodeGen::llvmCall(shared_ptr<Function> func, const expr_list args, bool excpt_check)
{
    return llvmCall(llvmFunction(func), func->type(), args, excpt_check);
}

llvm::Value* CodeGen::llvmCall(const string& name, const expr_list args, bool excpt_check)
{
    auto id = shared_ptr<ID>(new ID(name));
    auto expr = _hilti_module->body()->scope()->lookup(id);

    if ( ! expr )
        internalError(string("unknown function ") + id->name() + " in llvmCall()");

    if ( ! ast::isA<expression::Function>(expr) )
        internalError(string("ID ") + id->name() + " is not a function in llvmCall()");

    auto func = ast::as<expression::Function>(expr)->function();

    return llvmCall(func, args, excpt_check);
}

llvm::Value* CodeGen::llvmExtractBits(llvm::Value* value, llvm::Value* low, llvm::Value* high)
{
    auto width = llvm::cast<llvm::IntegerType>(value->getType())->getBitWidth();

    auto bits = builder()->CreateSub(llvmConstInt(width, width), high);
    bits = builder()->CreateAdd(bits, low);
    bits = builder()->CreateSub(bits, llvmConstInt(1, width));

    auto mask = builder()->CreateLShr(llvmConstInt(-1, width), bits);

    value = builder()->CreateLShr(value, low);

    return builder()->CreateAnd(value, mask);
}

llvm::Value* CodeGen::llvmLocationString(const Location& l)
{
    return llvmConstAsciizPtr(string(l).c_str());
}

llvm::Value* CodeGen::llvmCurrentLocation(const string& addl)
{
    string s = string(_stmt_builder->currentLocation());

    if ( addl.size() )
        s += " [" + addl + "]";

    return llvmConstAsciizPtr(s);
}

void CodeGen::llvmDtor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr, const string& location_addl)
{
    auto ti = typeInfo(type);

    if ( ti->dtor.size() == 0 && ti->dtor_func == 0 )
        return;

    // If we didn't get a pointer to the value, we need to create a tmp so
    // that we can take its address.
    if ( ! is_ptr ) {
        auto tmp = llvmAddTmp("gcobj", llvmType(type), val, false);
        val = tmp;
    }

    auto loc = llvmCurrentLocation(string("llvmDtor/") + location_addl);

    value_list args;
    args.push_back(llvmRtti(type));
    args.push_back(builder()->CreateBitCast(val, llvmTypePtr()));
    args.push_back(builder()->CreateBitCast(loc, llvmTypePtr()));
    llvmCallC("__hlt_object_dtor", args, false, false);
}

void CodeGen::llvmCctor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr, const string& location_addl)
{
    auto ti = typeInfo(type);

    if ( ti->cctor.size() == 0 && ti->cctor_func == 0 )
        return;

    // If we didn't get a pointer to the value, we need to create a tmp so
    // that we can take its address.
    if ( ! is_ptr ) {
        auto tmp = llvmAddTmp("gcobj", llvmType(type), val, false);
        val = tmp;
    }

    auto loc = llvmCurrentLocation(string("llvmCctor/") + location_addl);

    value_list args;
    args.push_back(llvmRtti(type));
    args.push_back(builder()->CreateBitCast(val, llvmTypePtr()));
    args.push_back(builder()->CreateBitCast(loc, llvmTypePtr()));
    llvmCallC("__hlt_object_cctor", args, false, false);
}

void CodeGen::llvmGCAssign(llvm::Value* dst, llvm::Value* val, shared_ptr<Type> type, bool plusone, bool dtor_first)
{
    assert(ast::isA<type::ValueType>(type));

    if ( dtor_first )
        llvmDtor(dst, type, true, "gc-assign");

    llvmCreateStore(val, dst);

    if ( ! plusone )
        llvmCctor(dst, type, true, "gc-assign");
}

void CodeGen::llvmGCClear(llvm::Value* val, shared_ptr<Type> type)
{
    assert(ast::isA<type::ValueType>(type));
    llvmDtor(val, type, true, "gc-clear");
}

void CodeGen::llvmDebugPrint(const string& stream, const string& msg)
{

    if ( debugLevel() == 0 )
        return;

    auto arg1 = llvmConstAsciizPtr(stream);
    auto arg2 = llvmConstAsciizPtr(msg);

    value_list args;
    args.push_back(arg1);
    args.push_back(arg2);

    llvmCallC("__hlt_debug_print", args, false, false);

#if 0
    builder::tuple::element_list elems;
    elems.push_back(builder::string::create(msg));

    auto arg1 = builder::string::create(stream);
    auto arg2 = builder::string::create("%s");
    auto arg3 = builder::tuple::create(elems);

    expr_list args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    llvmCall("hlt::debug_printf", args, false);
#endif
}

void CodeGen::llvmDebugPushIndent()
{
    value_list args = { llvmExecutionContext() };
    llvmCallC("__hlt_debug_push_indent", args, false, false);
}

void CodeGen::llvmDebugPopIndent()
{
    value_list args = { llvmExecutionContext() };
    llvmCallC("__hlt_debug_pop_indent", args, false, false);
}


llvm::Value* CodeGen::llvmSwitchEnumConst(llvm::Value* op, const case_list& cases, bool result, const Location& l)
{
    assert (op->getType()->isStructTy());

    // First check whether the enum has a value at all.
    //
    // FIXME: Copied from enum.cc, should factor out.
    auto flags = llvmExtractValue(op, 0);
    auto bit = builder()->CreateAnd(flags, llvmConstInt(HLT_ENUM_HAS_VAL, 8));
    auto have_val = builder()->CreateICmpNE(bit, llvmConstInt(0, 8));

    auto no_val = newBuilder("switch-no-val");
    auto cont = newBuilder("switch-do");
    llvmCreateCondBr(have_val, cont, no_val);

    pushBuilder(no_val);
    llvmRaiseException("Hilti::ValueError", l);
    popBuilder();

    pushBuilder(cont);
    auto switch_op = llvmExtractValue(op, 1);

    case_list ncases;

    for ( auto c : cases ) {
        assert(c._enums);

        std::list<llvm::ConstantInt*> nops;

        for ( auto op : c.op_enums ) {
            auto sval = llvm::cast<llvm::ConstantStruct>(op);
            auto val = llvmConstExtractValue(sval, 1);
            auto ival = llvm::cast<llvm::ConstantInt>(val);

            nops.push_back(llvmConstInt(ival->getZExtValue(), 64));
        }

        SwitchCase nc(c.label, nops, c.callback);
        ncases.push_back(nc);
    }

    return llvmSwitch(switch_op, ncases, result, l);
}

llvm::Value* CodeGen::llvmSwitch(llvm::Value* op, const case_list& cases, bool result, const Location& l)
{
    case_list ncases;
    const case_list* cases_to_use = &cases;

    assert(llvm::cast<llvm::IntegerType>(op->getType()));

    // If op is a constant, we prefilter the case list to directly remove all
    // cases that aren't matching.
    auto ci = llvm::dyn_cast<llvm::ConstantInt>(op);

    if ( ci ) {
        for ( auto c : cases ) {
            for ( auto op : c.op_integers ) {
                if ( llvm::cast<llvm::ConstantInt>(op)->getValue() == ci->getValue() ) {
                    ncases.push_back(c);
                    break;
                }
            }
        }

        cases_to_use = &ncases;
    }

    auto def = newBuilder("switch-default");
    auto cont = newBuilder("after-switch");
    auto switch_ = builder()->CreateSwitch(op, def->GetInsertBlock());

    std::list<std::pair<llvm::Value*, llvm::BasicBlock*>> returns;

    for ( auto c : *cases_to_use ) {
        assert(! c._enums);
        auto b = pushBuilder(::util::fmt("switch-%s", c.label.c_str()));
        auto r = c.callback(this);

        returns.push_back(std::make_pair(r, builder()->GetInsertBlock()));

        llvmCreateBr(cont);
        popBuilder();

        for ( auto op : c.op_integers )
            switch_->addCase(op, b->GetInsertBlock());
    }

    pushBuilder(def);
    llvmRaiseException("Hilti::ValueError", l);
    popBuilder();

    pushBuilder(cont); // Leave on stack.

    if ( ! result)
        return nullptr;

    assert(returns.size());

    auto phi = builder()->CreatePHI(returns.front().first->getType(), returns.size());

    for ( auto r : returns )
        phi->addIncoming(r.first, r.second);

    return phi;
}

static std::pair<int, shared_ptr<type::struct_::Field>> _getField(CodeGen* cg, shared_ptr<Type> type, const string& field)
{
    auto stype = ast::as<type::Struct>(type);

    if ( ! stype )
        cg->internalError("type is not a struct in _getField", type->location());

    int i = 0;

    for ( auto f : stype->fields() ) {
        if ( f->id()->name() == field )
            return std::make_pair(i, f);

        ++i;
    }

    cg->internalError(::util::fmt("unknown struct field name '%s' in _getField", field.c_str()), type->location());

    // Won't be reached.
    return std::make_pair(0, nullptr);
}

llvm::Value* CodeGen::llvmStructNew(shared_ptr<Type> type)
{
    auto stype = ast::as <type::Struct>(type);
    auto llvm_stype = llvmType(stype);

    if ( ! stype->fields().size() )
        // Empty struct are ok, we turn then into null pointers.
        return llvmConstNull(llvm_stype);

    auto s = llvmObjectNew(stype, llvm::cast<llvm::StructType>(llvm::cast<llvm::PointerType>(llvm_stype)->getElementType()));

    // Initialize fields
    auto zero = llvmGEPIdx(0);
    auto mask = 0;

    int j = 0;

    for ( auto f : stype->fields() ) {

        auto addr = llvmGEP(s, zero, llvmGEPIdx(j + 2));

        if ( f->default_() ) {
            // Initialize with default.
            mask |= (1 << j);
            auto llvm_default = llvmValue(f->default_(), f->type(), true);
            llvmGCAssign(addr, llvm_default, f->type(), true);
        }
        else
            // Initialize with null although we'll never access it. Better
            // safe than sorry ...
            llvmCreateStore(llvmConstNull(llvmType(f->type())), addr);

        ++j;
    }

    // Set mask.
    auto addr = llvmGEP(s, zero, llvmGEPIdx(1));
    llvmCreateStore(llvmConstInt(mask, 32), addr);

    return s;
}

llvm::Value* CodeGen::llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, int field, struct_get_default_callback_t default_, struct_get_filter_callback_t filter, const Location& l)
{
    auto i = ast::as<type::Struct>(stype)->fields().begin();
    std::advance(i, field);
    return llvmStructGet(stype, sval, (*i)->id()->name(), default_, filter, l);
}

llvm::Value* CodeGen::llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, struct_get_default_callback_t default_, struct_get_filter_callback_t filter, const Location& l)
{
    auto fd = _getField(this, stype, field);
    auto idx = fd.first;
    auto f = fd.second;

    // Check whether field is set.
    auto zero = llvmGEPIdx(0);
    auto addr = llvmGEP(sval, zero, llvmGEPIdx(1));
    auto mask = builder()->CreateLoad(addr);

    auto bit = llvmConstInt(1 << idx, 32);
    auto isset = builder()->CreateAnd(bit, mask);

    auto block_ok = newBuilder("ok");
    auto block_not_set = newBuilder("not_set");
    auto block_done = newBuilder("done");
    IRBuilder* ok_exit = block_ok;

    auto notzero = builder()->CreateICmpNE(isset, llvmConstInt(0, 32));
    llvmCreateCondBr(notzero, block_ok, block_not_set);


    pushBuilder(block_ok);

    // Load field
    addr = llvmGEP(sval, zero, llvmGEPIdx(idx + 2));
    llvm::Value* result_ok = builder()->CreateLoad(addr);

    if ( filter ) {
        result_ok = filter(this, result_ok);
        ok_exit = builder();
    }

    llvmCreateBr(block_done);
    popBuilder();

    pushBuilder(block_not_set);

    llvm::Value* def = nullptr;

    // Unset, raise exception if no default.

    if ( ! default_ )
        llvmRaiseException("Hilti::UndefinedValue", l);
    else
        def = default_(this);

    llvmCreateBr(block_done);
    popBuilder();

    pushBuilder(block_done);

    llvm::Value* result = nullptr;

    if ( default_ ) {
        auto phi = builder()->CreatePHI(result_ok->getType(), 2);
        phi->addIncoming(result_ok, ok_exit->GetInsertBlock());
        phi->addIncoming(def, block_not_set->GetInsertBlock());
        result = phi;
    }

    else
        result = result_ok;

    if ( ! filter )
        llvmCctor(result, f->type(), false, "struct-get");

    // Leave builder on stack.

    return result;
}

void CodeGen::llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, int field, shared_ptr<Expression> val)
{
    auto i = ast::as<type::Struct>(stype)->fields().begin();
    std::advance(i, field);
    return llvmStructSet(stype, sval, (*i)->id()->name(), val);
}

void CodeGen::llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, shared_ptr<Expression> val)
{
    auto fd = _getField(this, stype, field);
    auto lval = llvmCoerceTo(llvmValue(val), val->type(), fd.second->type());
    llvmStructSet(stype, sval, field, lval);
    llvmDtor(lval, fd.second->type(), false, "llvmStructSet");
}

void CodeGen::llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, int field, llvm::Value* val)
{
    auto i = ast::as<type::Struct>(stype)->fields().begin();
    std::advance(i, field);
    return llvmStructSet(stype, sval, (*i)->id()->name(), val);
}

void CodeGen::llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, llvm::Value* val)
{
    auto fd = _getField(this, stype, field);
    auto idx = fd.first;
    auto f = fd.second;

    // Set mask bit.
    auto zero = llvmGEPIdx(0);
    auto addr = llvmGEP(sval, zero, llvmGEPIdx(1));
    auto mask = builder()->CreateLoad(addr);
    auto bit = llvmConstInt(1 << idx, 32);
    auto new_ = builder()->CreateOr(bit, mask);
    llvmCreateStore(new_, addr);

    addr = llvmGEP(sval, zero, llvmGEPIdx(idx + 2));
    llvmGCAssign(addr, val, f->type(), false);
}

void CodeGen::llvmStructUnset(shared_ptr<Type> stype, llvm::Value* sval, int field)
{
    auto i = ast::as<type::Struct>(stype)->fields().begin();
    std::advance(i, field);
    return llvmStructUnset(stype, sval, (*i)->id()->name());
}

void CodeGen::llvmStructUnset(shared_ptr<Type> stype, llvm::Value* sval, const string& field)
{
    auto fd = _getField(this, stype, field);
    auto idx = fd.first;
    auto f = fd.second;

    // Clear mask bit.
    auto zero = llvmGEPIdx(0);
    auto addr = llvmGEP(sval, zero, llvmGEPIdx(1));
    auto mask = builder()->CreateLoad(addr);
    auto bit = llvmConstInt(~(1 << idx), 32);
    auto new_ = builder()->CreateAnd(bit, mask);
    llvmCreateStore(new_, addr);

    addr = llvmGEP(sval, zero, llvmGEPIdx(idx + 2));
    llvmGCClear(addr, f->type());
}

llvm::Value* CodeGen::llvmStructIsSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field)
{
    auto fd = _getField(this, stype, field);
    auto idx = fd.first;
    auto f = fd.second;

    // Check mask.
    auto zero = llvmGEPIdx(0);
    auto addr = llvmGEP(sval, zero, llvmGEPIdx(1));
    auto mask = builder()->CreateLoad(addr);
    auto bit = llvmConstInt(1 << idx, 32);
    auto isset = builder()->CreateAnd(bit, mask);
    auto notzero = builder()->CreateICmpNE(isset, llvmConstInt(0, 32));

    return notzero;
}

llvm::Value* CodeGen::llvmTupleElement(shared_ptr<Type> type, llvm::Value* tval, int idx, bool cctor)
{
    auto ttype = ast::as<type::Tuple>(type);
    assert(ttype);

    // ttype can be null if cctor is false.
    assert(ttype || ! cctor);

    shared_ptr<Type> elem_type;

    if ( ttype ) {
        int i = 0;
        for ( auto t : ttype->typeList() ) {
            if ( i++ == idx )
                elem_type = t;
        }
    }

    auto result = llvmExtractValue(tval, idx);

    if ( cctor )
        llvmCctor(result, elem_type, false, "tuple-element");

    return result;
}

llvm::Value* CodeGen::llvmIterBytesEnd()
{
    return llvmConstNull(llvmLibType("hlt.iterator.bytes"));
}

llvm::Value* CodeGen::llvmMalloc(llvm::Type* ty, const string& type, const Location& l)
{
    value_list args { llvmSizeOf(ty), llvmConstAsciizPtr(type), llvmConstAsciizPtr(l) };
    auto result = llvmCallC("__hlt_malloc", args, false, false);
    return builder()->CreateBitCast(result, llvmTypePtr(ty));
}

llvm::Value* CodeGen::llvmMalloc(llvm::Value* size, const string& type, const Location& l)
{
    value_list args { builder()->CreateZExt(size, llvmTypeInt(64)), llvmConstAsciizPtr(type), llvmConstAsciizPtr(l) };
    auto result = llvmCallC("__hlt_malloc", args, false, false);
    return builder()->CreateBitCast(result, llvmTypePtr());
}

void CodeGen::llvmFree(llvm::Value* val, const string& type, const Location& l)
{
    val = builder()->CreateBitCast(val, llvmTypePtr());
    value_list args { val, llvmConstAsciizPtr(type), llvmConstAsciizPtr(l) };
    llvmCallC("__hlt_free", args, false, false);
}

llvm::Value* CodeGen::llvmObjectNew(shared_ptr<Type> type, llvm::StructType* llvm_type)
{
    value_list args { llvmRtti(type), llvmSizeOf(llvm_type), llvmConstAsciizPtr("llvm.object.new") };
    auto result = llvmCallC("__hlt_object_new", args, false, false);
    return builder()->CreateBitCast(result, llvmTypePtr(llvm_type));
}

shared_ptr<Type> CodeGen::typeByName(const string& name)
{
    auto expr = _hilti_module->body()->scope()->lookup(std::make_shared<ID>(name));

    if ( ! expr )
        internalError(string("unknown type ") + name + " in typeByName()");

    if ( ! ast::isA<expression::Type>(expr) )
        internalError(string("ID ") + name + " is not a type in typeByName()");

    return ast::as<expression::Type>(expr)->typeValue();
}

   /// Creates a tuple from a givem list of elements. The returned tuple will
   /// have the cctor called for all its members.
   ///
   /// elems: The tuple elements.
   ///
   /// Returns: The new tuple.
llvm::Value* CodeGen::llvmTuple(shared_ptr<Type> type, const element_list& elems, bool cctor)
{
    auto ttype = ast::as<type::Tuple>(type);
    auto t = ttype->typeList().begin();

    value_list vals;

    for ( auto e : elems ) {
        auto op = builder::codegen::create(e.first, e.second);
        vals.push_back(llvmValue(op, *t++, cctor));
    }

    return llvmTuple(vals);
}

llvm::Value* CodeGen::llvmTuple(shared_ptr<Type> type, const expression_list& elems, bool cctor)
{
    auto ttype = ast::as<type::Tuple>(type);
    auto e = elems.begin();

    value_list vals;

    for ( auto t : ttype->typeList() )
        vals.push_back(llvmValue(*e++, t, cctor));

    return llvmTuple(vals);
}

llvm::Value* CodeGen::llvmTuple(const value_list& elems)
{
    return llvmValueStruct(elems);
}

llvm::Value* CodeGen::llvmClassifierField(shared_ptr<Type> field_type, shared_ptr<Type> src_type, llvm::Value* src_val, const Location& l)
{
    return _field_builder->llvmClassifierField(field_type, src_type, src_val, l);
}

llvm::Value* CodeGen::llvmClassifierField(llvm::Value* data, llvm::Value* len, llvm::Value* bits, const Location& l)
{
    auto ft = llvmLibType("hlt.classifier.field");

    llvm::Value* size = llvmSizeOf(ft);
    size = builder()->CreateAdd(size, builder()->CreateZExt(len, size->getType()));
    auto field = llvmMalloc(size, "hlt.classifier.field", l);
    field = builder()->CreateBitCast(field, llvmTypePtr(ft));

    if ( ! bits )
        bits = builder()->CreateMul(len, llvmConstInt(8, 64));

    // Initialize the static attributes of the field.
    llvm::Value* s = llvmConstNull(ft);
    s = llvmInsertValue(s, len, 0);
    s = llvmInsertValue(s, bits, 1);

    llvmCreateStore(s, field);

    // Copy the data bytes into the field.
    if ( data ) {
        data = builder()->CreateBitCast(data, llvmTypePtr());
        auto dst = llvmGEP(field, llvmGEPIdx(0), llvmGEPIdx(2));
        dst = builder()->CreateBitCast(dst, llvmTypePtr());
        llvmMemcpy(dst, data, len);
    }
    else {
        assert(len==0);
    }

    return builder()->CreateBitCast(field, llvmTypePtr());
}

llvm::Value* CodeGen::llvmHtoN(llvm::Value* val)
{
    auto itype = llvm::cast<llvm::IntegerType>(val->getType());
    assert(itype);

    int width = itype->getBitWidth();
    const char* f = "";

    switch ( width ) {
     case 8:
        return val;

     case 16:
        f = "hlt::hton16";
        break;

     case 32:
        f = "hlt::hton32";
        break;

     case 64:
        f = "hlt::hton64";
        break;

     default:
        internalError("unexpected bit width in llvmNtoH");
    }

    expr_list args = { builder::codegen::create(builder::integer::type(width), val) };
    return llvmCall(f, args);
}

llvm::Value* CodeGen::llvmNtoH(llvm::Value* val)
{
    auto itype = llvm::cast<llvm::IntegerType>(val->getType());
    assert(itype);

    int width = itype->getBitWidth();
    const char* f = "";

    switch ( width ) {
     case 8:
        return val;

     case 16:
        f = "hlt::ntoh16";
        break;

     case 32:
        f = "hlt::ntoh32";
        break;

     case 64:
        f = "hlt::ntoh64";
        break;

     default:
        internalError("unexpected bit width in llvmHtoN");
    }

    expr_list args = { builder::codegen::create(builder::integer::type(width), val) };
    return llvmCall(f, args);
}

void CodeGen::llvmMemcpy(llvm::Value *dst, llvm::Value *src, llvm::Value *n)
{
    src = builder()->CreateBitCast(src, llvmTypePtr());
    dst = builder()->CreateBitCast(dst, llvmTypePtr());
    n = builder()->CreateZExt(n, llvmTypeInt(64));

    CodeGen::value_list args = { dst, src, n, llvmConstInt(1, 32), llvmConstInt(0, 1) };
    std::vector<llvm::Type *> tys = { llvmTypePtr(), llvmTypePtr(), llvmTypeInt(64) };

    llvmCallIntrinsic(llvm::Intrinsic::memcpy, tys, args);
}

void CodeGen::llvmInstruction(shared_ptr<Instruction> instr, shared_ptr<Expression> op1, shared_ptr<Expression> op2, shared_ptr<Expression> op3, const Location& l)
{
    return llvmInstruction(nullptr, instr, op1, op2, op3, l);
}

void CodeGen::llvmInstruction(shared_ptr<Expression> target, shared_ptr<Instruction> instr, shared_ptr<Expression> op1, shared_ptr<Expression> op2, shared_ptr<Expression> op3, const Location& l)
{
    auto name = instr->id()->name();

    if ( ::util::startsWith(name, ".op.") ) {
        // These are dummy instructions used only to provide a single class
        // for the builder interface to access overloaded operators. We use
        // the non-prefixed name instead to do the lookup by name.
        name = name.substr(4, std::string::npos);
        return llvmInstruction(target, name, op1, op2, op3);
    }

    instruction::Operands ops = { target, op1, op2, op3 };
    auto id = std::make_shared<ID>(name);
    auto matches = InstructionRegistry::globalRegistry()->getMatching(id, ops);

    auto resolved = InstructionRegistry::globalRegistry()->resolveStatement(instr, ops);
    assert(resolved);

    _stmt_builder->llvmStatement(resolved);
}

void CodeGen::llvmInstruction(shared_ptr<Expression> target, const string& mnemo, shared_ptr<Expression> op1, shared_ptr<Expression> op2, shared_ptr<Expression> op3, const Location& l)
{
    instruction::Operands ops = { target, op1, op2, op3 };

    auto id = std::make_shared<ID>(mnemo);
    auto matches = InstructionRegistry::globalRegistry()->getMatching(id, ops);

    if ( matches.size() != 1 ) {
        fprintf(stderr, "target: %s\n", target ? target->type()->render().c_str() : "(null)");
        fprintf(stderr, "op1   : %s\n", op1 ? op1->type()->render().c_str() : "(null)");
        fprintf(stderr, "op2   : %s\n", op2 ? op2->type()->render().c_str() : "(null)");
        fprintf(stderr, "op3   : %s\n", op3 ? op3->type()->render().c_str() : "(null)");
        internalError(::util::fmt("llvmInstruction: %d matches for mnemo %s", matches.size(), mnemo.c_str()));
    }

    auto resolved = InstructionRegistry::globalRegistry()->resolveStatement(matches.front(), ops);
    assert(resolved);

    _stmt_builder->llvmStatement(resolved);
}

shared_ptr<hilti::Expression> CodeGen::makeLocal(const string& name, shared_ptr<Type> type)
{
    string n = "__" + name;

    int idx = 1;
    string unique_name = name;

    while ( _functions.back()->locals.find(unique_name) != _functions.back()->locals.end() )
        unique_name = ::util::fmt("%s.%d", n.c_str(), ++idx);

    llvmAddLocal(unique_name, type);

    auto id = std::make_shared<ID>(unique_name);
    auto var = std::make_shared<variable::Local>(id, type);
    auto expr = std::make_shared<expression::Variable>(var);

    var->setInternalName(unique_name);

    return expr;
}

std::pair<bool, IRBuilder*> CodeGen::topEndOfBlockHandler()
{
    if ( ! _functions.back()->handle_block_end.size() )
        return std::make_pair(true, (IRBuilder*)nullptr);

    auto handler = _functions.back()->handle_block_end.back();
    return std::make_pair(handler != nullptr, handler);
}

void CodeGen::llvmBlockingInstruction(statement::Instruction* i, try_func try_, finish_func finish,
                                      shared_ptr<Type> blockable_ty, llvm::Value* blockable_val)
{
    auto loop = newBuilder("blocking-try");
    auto yield_ = newBuilder("blocking-yield");
    auto done = newBuilder("blocking-finish");

    llvmCreateBr(loop);

    pushBuilder(loop);
    auto result = try_(this, i);

    auto blocked = llvmMatchException("Hilti::WouldBlock", llvmCurrentException());
    llvmCreateCondBr(blocked, yield_, done);
    popBuilder();

    pushBuilder(yield_);
    llvmClearException();
    auto fiber = llvmCurrentFiber();
    llvmFiberYield(fiber, blockable_ty, blockable_val);
    llvmCreateBr(loop);
    popBuilder();

    pushBuilder(done);
    llvmCheckException();
    finish(this, i, result);

    // Leave on stack.
}

void CodeGen::llvmProfilerStart(llvm::Value* tag, llvm::Value* style, llvm::Value* param, llvm::Value* tmgr)
{
    assert(tag);

    if ( _profile_level == 0 )
        return;

    if ( ! style )
        style = llvmEnum("Hilti::ProfileStyle::Standard");

    if ( ! param )
        param = llvmConstInt(0, 64);

    if ( ! tmgr ) {
        auto rtmgr = builder::reference::type(builder::timer_mgr::type());
        tmgr = llvmConstNull(llvmType(rtmgr));
    }

    auto eexpr = _hilti_module->body()->scope()->lookup((std::make_shared<ID>("Hilti::ProfileStyle")));
    assert(eexpr);

    expr_list args = {
        builder::codegen::create(builder::string::type(), tag),
        builder::codegen::create(ast::checkedCast<expression::Type>(eexpr)->typeValue(), style),
        builder::codegen::create(builder::integer::type(64), param),
        builder::codegen::create(builder::reference::type(builder::timer_mgr::type()), tmgr)
    };

    llvmCall("hlt::profiler_start", args, false);
}

void CodeGen::llvmProfilerStart(const string& tag, const string& style, int64_t param, llvm::Value* tmgr)
{
    assert(tag.size());

    auto ltag = llvmString(tag);
    auto lstyle = style.size() ? llvmEnum(style) : static_cast<llvm::Value*>(nullptr);
    auto lparam = llvmConstInt(param, 64);

    llvmProfilerStart(ltag, lstyle, lparam, tmgr);
}

void CodeGen::llvmProfilerStop(llvm::Value* tag)
{
    assert(tag);

    if ( _profile_level == 0 )
        return;

    expr_list args = {
        builder::codegen::create(builder::string::type(), tag),
    };

    llvmCall("hlt::profiler_stop", args, false);
}

void CodeGen::llvmProfilerStop(const string& tag)
{
    assert(tag.size());

    auto ltag = llvmString(tag);
    llvmProfilerStop(ltag);
}

void CodeGen::llvmProfilerUpdate(llvm::Value* tag, llvm::Value* arg)
{
    assert(tag);

    if ( _profile_level == 0 )
        return;

    if ( ! arg )
        arg = llvmConstInt(0, 64);

    expr_list args = {
        builder::codegen::create(builder::string::type(), tag),
        builder::codegen::create(builder::integer::type(64), arg),
    };

    llvmCall("hlt::profiler_update", args, false);
}

void CodeGen::llvmProfilerUpdate(const string& tag, int64_t arg)
{
    assert(tag.size());

    auto ltag = llvmString(tag);
    auto larg = llvmConstInt(arg, 64);

    llvmProfilerUpdate(ltag, larg);
}


