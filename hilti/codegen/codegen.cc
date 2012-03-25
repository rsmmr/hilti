
#include <util/util.h>

#include "../module.h"

#include "codegen.h"
#include "util.h"

#include "loader.h"
#include "storer.h"
#include "unpacker.h"
#include "coercer.h"
#include "builder.h"
#include "stmt-builder.h"
#include "type-builder.h"
#include "debug-info-builder.h"
#include "passes/collector.h"

using namespace hilti;
using namespace codegen;

CodeGen::CodeGen(const path_list& libdirs, int cg_debug_level)
    : _coercer(new Coercer(this)),
      _loader(new Loader(this)),
      _storer(new Storer(this)),
      _unpacker(new Unpacker(this)),
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
    _profile = profile;

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

        createInitFunction();

        initGlobals();

        // Kick-off recursive code generation.
        _stmt_builder->llvmStatement(hltmod->body());

        finishInitFunction();

        createGlobalsInitFunction();

        createLinkerData();

        createRtti();

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
    for ( auto arg = function()->arg_begin(); arg != function()->arg_end(); ++arg ) {
        if ( arg->getName() == symbols::ArgExecutionContext )
            return arg;
    }

    internalError("no context argument found in llvmExecutionContext");
    return 0;
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

void CodeGen::llvmStore(shared_ptr<hilti::Expression> target, llvm::Value* value)
{
    _storer->llvmStore(target, value, true);
}

std::pair<llvm::Value*, llvm::Value*> CodeGen::llvmUnpack(
                   shared_ptr<Type> type, shared_ptr<Expression> begin,
                   shared_ptr<Expression> end, shared_ptr<Expression> fmt,
                   shared_ptr<Expression> arg, const Location& location)
{
    UnpackArgs args;
    args.type = type;
    args.begin = begin ? llvmValue(begin) : nullptr;
    args.end = end ? llvmValue(end) : nullptr;
    args.fmt = fmt ? llvmValue(fmt) : nullptr;
    args.arg = arg ? llvmValue(arg) : nullptr;
    args.location = location;

    UnpackResult result = _unpacker->llvmUnpack(args);

    auto val = builder()->CreateLoad(result.value_ptr);
    auto iter = builder()->CreateLoad(result.iter_ptr);

    return std::make_pair(val, iter);
}

std::pair<llvm::Value*, llvm::Value*> CodeGen::llvmUnpack(
                   shared_ptr<Type> type, llvm::Value* begin,
                   llvm::Value* end, llvm::Value* fmt,
                   llvm::Value* arg, const Location& location)
{
    UnpackArgs args;
    args.type = type;
    args.begin = begin;
    args.end = end;
    args.fmt = fmt;
    args.arg = arg;
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
        if ( arg->getName() == name )
            return arg;
    }

    internalError("unknown parameter name " + name);
    return 0; // Never reached.
}

void CodeGen::llvmStore(statement::Instruction* instr, llvm::Value* value)
{
    _storer->llvmStore(instr->target(), value, true);
}

llvm::Function* CodeGen::pushFunction(llvm::Function* function, bool push_builder)
{
    unique_ptr<FunctionState> state(new FunctionState);
    state->function = function;
    _functions.push_back(std::move(state));

    if ( push_builder )
        pushBuilder("entry");

    return function;
}

void CodeGen::popFunction()
{
    if ( ! block()->getTerminator() )
        // Add a return if we don't have one yet.
        builder()->CreateRetVoid();

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

    pushFunction(_module_init_func);
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

    pushFunction(_globals_init_func);

    // Init the global string constants.
    for ( auto gs : _global_strings ) {
        auto str = gs.first;
        auto glob = gs.second;

        std::vector<llvm::Constant*> vec_data;
        for ( auto c : str )
            vec_data.push_back(llvmConstInt(c, 8));

        auto array = llvmConstArray(llvmTypeInt(8), vec_data);
        llvm::Constant* data = llvmAddConst("string", array);
        data = llvm::ConstantExpr::getBitCast(data, llvmTypePtr());

        value_list args;
        args.push_back(data);
        args.push_back(llvmConstInt(str.size(), 64));
        auto s = llvmCallC(llvmLibFunction("hlt_string_from_data"), args, true);
        builder()->CreateStore(s, glob);
    }

    // Init user defined globals.
    for ( auto g : _collector->globals() ) {
        auto init = g->init() ? llvmValue(g->init(), g->type(), true) : llvmInitVal(g->type());
        assert(init);
        auto addr = llvmGlobal(g.get());
        llvmCreateStore(init, addr);
    }

    if ( ! functionEmpty() )
        // Add a terminator to the function.
        builder()->CreateRetVoid();

    else {
        // We haven't added anything to the function, just discard.
        _globals_init_func->removeFromParent();
        _globals_init_func = nullptr;
    }

    llvmBuildExitBlock();

    popFunction();

    // Create a function that that destroys all the memory managed objects in
    // there.
    name = util::mangle(_hilti_module->id(), true, nullptr, "dtor.globals");
    _globals_dtor_func = llvmAddFunction(name, llvmTypeVoid(), no_params, false, type::function::HILTI_C);

    pushFunction(_globals_dtor_func);

    for ( auto g : _collector->globals() ) {
        auto val = llvmGlobal(g);
        llvmDtor(val, g->type(), true);
    }

    auto stype = shared_ptr<Type>(new type::String());

    for ( auto gs : _global_strings ) {
        llvmDtor(gs.second, stype, true);
    }

    if ( ! functionEmpty() )
        // Add a terminator to the function.
        builder()->CreateRetVoid();

    else {
        // We haven't added anything to the function, just discard.
        _globals_dtor_func->removeFromParent();
        _globals_dtor_func = nullptr;
    }

    llvmBuildExitBlock();

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

shared_ptr<codegen::TypeInfo> CodeGen::typeInfo(shared_ptr<hilti::Type> type)
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

llvm::Constant* CodeGen::llvmRtti(shared_ptr<hilti::Type> type)
{
    llvm::Constant* val = lookupCachedConstant("type.rtti", type->render());

    if ( val ) {
        auto v = llvm::cast<llvm::GlobalVariable>(val);
        assert(v);
        return v->getInitializer();
    }

    // We add the global here first and cache it before we call llvmRtti() so
    // the recursive calls function properly.
    string name = util::mangle(string("hlt_type_info_hlt_") + type->render(), true, nullptr, "", false);
    name = ::util::strreplace(name, "_ref", "");

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

    return constant1->getInitializer();
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

        auto glob = lookupCachedConstant("type-exception", excpt->render());

        if ( glob )
            return  glob;

        assert(excpt->id());

        auto name = llvmConstAsciizPtr(excpt->id()->pathAsString());
        auto base = llvmExceptionTypeObject(ast::as<type::Exception>(excpt->baseType()));
        auto arg  = excpt->argType() ? llvmRtti(excpt->argType()) : llvmConstNull(llvmTypePtr(llvmTypeRtti()));

        base = llvm::ConstantExpr::getBitCast(base, llvmTypePtr());
        arg = llvm::ConstantExpr::getBitCast(arg, llvmTypePtr());

        constant_list elems;
        elems.push_back(name);
        elems.push_back(base);
        elems.push_back(arg);
        auto val = llvmConstStruct(llvmLibType("hlt.exception.type"), elems);
        glob = llvmAddConst("exception", val);

        return cacheConstant("type-exception", excpt->render(), glob);
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

llvm::Value* CodeGen::llvmStringPtr(const string& s)
{
    if ( s.size() == 0 )
        // The empty string is represented by a null pointer.
        return llvmConstNull(llvmTypeString());

    // See if we have already created an array for this string.
    llvm::Value* data = lookupCachedValue("llvmString", s);

    if ( ! data ) {
        data = llvmAddGlobal("string", llvmTypeString());
        cacheValue("llvmString", s, data);
        _global_strings.push_back(make_pair(s, data));
    }

    return data;
}

llvm::Value* CodeGen::llvmString(const string& s)
{
    return builder()->CreateLoad(llvmStringPtr(s));
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

    if ( ! init )
        init = typeInfo(type)->init_val;

    llvm::BasicBlock& block(function()->getEntryBlock());

    auto builder = newBuilder(&block, true);

    auto local = builder->CreateAlloca(llvm_type, 0, name);

    if ( init )
        // Use current builder here.
        llvmCreateStore(init, local);

    _functions.back()->locals.insert(make_pair(name, std::make_pair(local, type)));

    delete builder;

    return local;
}

llvm::Value* CodeGen::llvmAddTmp(const string& name, llvm::Type* type, llvm::Value* init, bool reuse)
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

llvm::Function* CodeGen::llvmAddFunction(const string& name, llvm::Type* rtype, parameter_list params, bool internal, type::function::CallingConvention cc)
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
        auto arg_llvm_type = llvmType(p.second);

        switch ( cc ) {
         case type::function::HILTI:
         case type::function::HOOK:
            llvm_args.push_back(make_pair(p.first, arg_llvm_type));
            break;

         case type::function::HILTI_C: {
            auto ptype = p.second;
            shared_ptr<TypeInfo> pti = typeInfo(ptype);

            if ( pti->pass_type_info ) {
                llvm_args.push_back(make_tuple(string("ti_") + p.first, llvmTypePtr(llvmTypeRtti())));
                llvm_args.push_back(make_tuple(p.first, llvmTypePtr()));
            }

            else
                llvm_args.push_back(make_tuple(p.first, arg_llvm_type));

             break;
         }
         case type::function::C:
            llvm_args.push_back(make_tuple(p.first, arg_llvm_type));
            break;

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
        llvm_args.push_back(std::make_pair(symbols::ArgExecutionContext, llvmTypePtr(llvmTypeExecutionContext())));
        break;

     case type::function::C:
        break;

     default:
        internalError("unknown calling convention in llvmAddFunction");
    }

    std::vector<llvm::Type*> func_args;
    for ( auto a : llvm_args )
        func_args.push_back(a.second);

    auto ftype = llvm::FunctionType::get(rtype, func_args, false);
    func = llvm::Function::Create(ftype, llvm_linkage, name, _module);

    func->setCallingConv(llvm_cc);

    auto i = llvm_args.begin();
    for ( auto a = func->arg_begin(); a != func->arg_end(); ++a, ++i )
        a->setName(i->first);

    return func;
}

llvm::Function* CodeGen::llvmAddFunction(const string& name, llvm::Type* rtype, llvm_parameter_list params, bool internal)
{
    auto mangled_name = util::mangle(name, true, nullptr, "", false);

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

llvm::Function* CodeGen::llvmAddFunction(shared_ptr<Function> func, bool internal, type::function::CallingConvention cc, const string& force_name)
{
    if ( cc == type::function::DEFAULT )
        cc = func->type()->callingConvention();

    parameter_list params;

    for ( auto p : func->type()->parameters() )
        params.push_back(make_pair(p->id()->name(), p->type()));

    auto name = force_name.size() ? force_name : util::mangle(func->id(), true, func->module()->id(), "", internal);

    auto rtype = llvmType(func->type()->result()->type());

    return llvmAddFunction(name, rtype, params, internal, cc);
}

llvm::Function* CodeGen::llvmFunction(shared_ptr<Function> func, bool force_new)
{
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

void CodeGen::llvmReturn(llvm::Value* result)
{
    auto state = _functions.back().get();

    if ( ! state->exit_block )
        state->exit_block = llvm::BasicBlock::Create(llvmContext(), ".exit", function());

    builder()->CreateBr(state->exit_block);

    if ( result )
        state->exits.push_back(std::make_pair(block(), result));
}

void CodeGen::llvmBuildExitBlock()
{
    auto state = _functions.back().get();

    if ( ! state->exit_block )
        return;

    auto exit_builder = newBuilder(state->exit_block);

    llvm::PHINode* phi = nullptr;

    if ( state->exits.size() ) {
        auto rtype = state->function->getReturnType();
        phi = exit_builder->CreatePHI(rtype, state->exits.size());

        for ( auto e : state->exits )
            phi->addIncoming(e.second, e.first);
    }

    pushBuilder(exit_builder);
    llvmBuildFunctionCleanup();
    popBuilder();

    if ( phi )
        exit_builder->CreateRet(phi);
    else
        exit_builder->CreateRetVoid();

    delete exit_builder;
}

void CodeGen::llvmBuildFunctionCleanup()
{
    // Unref locals (incl. parameters).
    for ( auto l : _functions.back()->locals )
        llvmDtor(l.second.first, l.second.second, true);
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

    if ( excpt_check )
        llvmCheckException();

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

void CodeGen::llvmRaiseException(const string& exception, const Location& l, llvm::Value* arg)
{
    auto expr = _hilti_module->body()->scope()->lookup((std::make_shared<ID>(exception)));

    if ( ! expr )
        internalError(::util::fmt("unknown exception %s", exception.c_str()), l);

    auto type = ast::as<expression::Type>(expr)->typeValue();
    assert(type);

    auto etype = ast::as<type::Exception>(type);
    assert(etype);

    CodeGen::value_list args;
    args.push_back(llvmExceptionTypeObject(etype));
    args.push_back(llvmConstNull());
    args.push_back(llvmLocationString(l));

    auto excpt = llvmCallC("hlt_exception_new", args, false, false);

    llvmRaiseException(excpt);
}

void CodeGen::llvmRaiseException(llvm::Value* excpt)
{
    value_list args;
    args.push_back(excpt);
    args.push_back(llvmExecutionContext());
    llvmCallC("__hlt_context_set_exception", args, false, false);
    llvmTriggerExceptionHandling(true);
}

void CodeGen::llvmRethrowException()
{
    // This simply returns with a null value. The caller will check for a thrown exception.

    auto rt = _functions.back()->function->getReturnType();

    if ( rt->isVoidTy() )
        llvmReturn();
    else
        llvmReturn(llvmConstNull(rt));
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

void CodeGen::llvmCheckException()
{
    llvmTriggerExceptionHandling(false);
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
            llvmCreateCondBr(is_null, cont, _functions.back()->catches.front());
        }

        else
            llvmCreateBr(_functions.back()->catches.front());
    }

    else {
        // No catches at all, unwind.
        auto unwind = haveBuilder("unwind");

        if ( ! unwind ) {
            unwind = pushBuilder("unwind");
            llvmRethrowException();
            popBuilder();
        }

        if ( ! known_exception ) {
            cont = newBuilder("no-excpt");
            llvmCreateCondBr(is_null, cont, unwind);
        }

        else
            llvmCreateBr(unwind);
    }

    if ( cont )
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
    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return builder()->CreateInsertValue(aggr, val, vec);
}

llvm::Constant* CodeGen::llvmConstInsertValue(llvm::Constant* aggr, llvm::Constant* val, unsigned int idx)
{
    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return llvm::ConstantExpr::getInsertValue(aggr, val, vec);
}

llvm::Value* CodeGen::llvmExtractValue(llvm::Value* aggr, unsigned int idx)
{
    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return builder()->CreateExtractValue(aggr, vec);
}

llvm::Constant* CodeGen::llvmConstExtractValue(llvm::Constant* aggr, unsigned int idx)
{
    std::vector<unsigned int> vec;
    vec.push_back(idx);
    return llvm::ConstantExpr::getExtractValue(aggr, vec);
}

void CodeGen::llvmBuildCWrapper(shared_ptr<Function> func, llvm::Function* internal)
{
    auto ftype = func->type();

    if ( ! func->body() )
        // No implementation, nothing to do here.
        return;

    assert(ftype->callingConvention() == type::function::HILTI);
    assert(func->body());

    auto name = util::mangle(func->id(), true, nullptr, "", false);
    auto llvm_func = llvmAddFunction(func, false, type::function::HILTI_C);

    auto args = llvm_func->arg_begin();
    auto excpt = args;

    pushFunction(llvm_func);

    expr_list params;
    for ( auto p : ftype->parameters() ) {
        auto expr = shared_ptr<Expression>(new expression::Parameter(p));
        params.push_back(expr);
    }

    auto result = llvmCall(func, params, false); // No excpt check, we pass it right on.

    // Copy exception over.
    auto ctx_excpt = llvmCurrentException();
    llvmCreateStore(ctx_excpt, excpt);

    if ( ftype->result()->type()->equal(shared_ptr<Type>(new type::Void())) )
        llvmReturn();
    else
        llvmReturn(result);

    llvmBuildExitBlock();

    popFunction();
}

llvm::CallInst* CodeGen::llvmCall(llvm::Value* llvm_func, shared_ptr<type::Function> ftype, const expr_list& args, bool excpt_check)
{
    auto result = llvmDoCall(llvm_func, nullptr, ftype, args, nullptr, excpt_check);
    auto ci = llvm::cast<llvm::CallInst>(result);
    assert(ci);
    return ci;
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
        shared_ptr<TypeInfo> pti = typeInfo(ptype);

        auto coerced = (*arg)->coerceTo(ptype);
        auto arg_type = coerced->type();
        auto arg_llvm_type = llvmType(arg_type);
        auto arg_value = llvmValue(coerced);

        ++arg;

        switch ( ftype->callingConvention() ) {
         case type::function::HILTI:
         case type::function::HOOK:
            // Can pass directly but need context.
            llvm_args.push_back(arg_value);
            break;

         case type::function::HILTI_C: {
            if ( pti->pass_type_info ) {
                auto rtti = llvmRtti(arg_type);
                llvm_args.push_back(rtti);
                auto tmp = llvmAddTmp("arg", arg_llvm_type, arg_value);
                llvm_args.push_back(builder()->CreateBitCast(tmp, llvmTypePtr()));
            }
            else
                llvm_args.push_back(arg_value);

            break;
         }

         case type::function::C:
            // Don't mess with arguments.
            llvm_args.push_back(arg_value);
            break;

         default:
            internalError("unknown calling convention in llvmCall");
        }
    }

    // Add additional parameters our calling convention may need.

    llvm::Value* excpt = nullptr;

    switch ( ftype->callingConvention() ) {
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

    // Do the call.

    auto result = llvmCreateCall(llvm_func, llvm_args);

    if ( excpt_check )
        llvmCheckException();

    // Retrieve return value according to calling convention.

    switch ( ftype->callingConvention() ) {
     case type::function::HILTI:
        // Can take it directly.
        break;

     case type::function::HOOK:
        break;

     case type::function::HILTI_C:
        assert(excpt);
        // assert(false && "Do ABI stuff");
        break;

     case type::function::C:
        // Don't mess with result.
        break;

     default:
        internalError("unknown calling convention in llvmCall");
    }

    if ( ! hook ) {
        auto rtype = ftype->result()->type();
        llvmCctor(result, rtype, false);
    }

    return result;
}

llvm::CallInst* CodeGen::llvmCall(shared_ptr<Function> func, const expr_list& args, bool excpt_check)
{
    return llvmCall(llvmFunction(func), func->type(), args, excpt_check);
}

llvm::CallInst* CodeGen::llvmCall(const string& name, const expr_list& args, bool excpt_check)
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

void CodeGen::llvmCheckCException(llvm::Value* excpt)
{
    // Not yet implemented.
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

void CodeGen::llvmDtorAfterInstruction(llvm::Value* val, shared_ptr<Type> type, bool is_ptr)
{
    _functions.back()->dtors_after_ins.push_back(std::make_tuple(std::make_tuple(val, is_ptr), type));
}

void CodeGen::finishStatement()
{
    // Unref values registered for doing so.
    for ( auto unref : _functions.back()->dtors_after_ins )
        llvmDtor(unref.first.first, unref.second, unref.first.second);

    _functions.back()->dtors_after_ins.clear();
}

llvm::Value* CodeGen::llvmLocationString(const Location& l)
{
    return llvmConstAsciizPtr(string(l).c_str());
}

void CodeGen::llvmDtor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr)
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

    value_list args;
    args.push_back(llvmRtti(type));
    args.push_back(builder()->CreateBitCast(val, llvmTypePtr()));
    args.push_back(builder()->CreateBitCast(llvmConstAsciizPtr("HILTI: llvmDtor"), llvmTypePtr()));
    llvmCallC("__hlt_object_dtor", args, false, false);
}

void CodeGen::llvmCctor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr)
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

    value_list args;
    args.push_back(llvmRtti(type));
    args.push_back(builder()->CreateBitCast(val, llvmTypePtr()));
    args.push_back(builder()->CreateBitCast(llvmConstAsciizPtr("HILTI: llvmCctor"), llvmTypePtr()));
    llvmCallC("__hlt_object_cctor", args, false, false);
}

void CodeGen::llvmGCAssign(llvm::Value* dst, llvm::Value* val, shared_ptr<Type> type, bool plusone)
{
    assert(ast::isA<type::ValueType>(type));
    llvmDtor(dst, type, true);
    llvmCreateStore(val, dst);

    if ( ! plusone )
        llvmCctor(dst, type, true);
}

void CodeGen::llvmGCClear(llvm::Value* val, shared_ptr<Type> type)
{
    assert(ast::isA<type::ValueType>(type));
    llvmDtor(val, type, true);
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

llvm::Value* CodeGen::llvmSwitchEnumConst(llvm::Value* op, const case_list& cases, bool result, const Location& l)
{
    case_list ncases;

    for ( auto c : cases ) {
        assert(c._enums);

        std::list<llvm::Value*> nops;

        for ( auto op : c.op_enums ) {
            auto sval = llvm::cast<llvm::ConstantStruct>(op);
            auto val = llvmConstExtractValue(sval, 1);
            auto ival = llvm::cast<llvm::ConstantInt>(val);

            nops.push_back(llvmConstInt(ival->getZExtValue(), 64));
        }

        SwitchCase nc(c.label, nops, c.callback);
        ncases.push_back(nc);
    }

    return llvmSwitch(op, ncases, result, l);
}

llvm::Value* CodeGen::llvmSwitch(llvm::Value* op, const case_list& cases, bool result, const Location& l)
{
    assert(llvm::cast<llvm::IntegerType>(op->getType()));

    auto def = pushBuilder("switch-default");
    llvmRaiseException("hlt_exception_value_error", l);
    popBuilder();

    auto cont = newBuilder("after-switch");
    auto switch_ = builder()->CreateSwitch(op, def->GetInsertBlock());

    std::list<std::pair<llvm::Value*, llvm::BasicBlock*>> returns;

    for ( auto c : cases ) {
        assert(! c._enums);
        auto b = pushBuilder(::util::fmt("switch-%s", c.label.c_str()));
        auto r = c.callback(this);
        llvmCreateBr(cont);
        popBuilder();

        returns.push_back(std::make_pair(r, b->GetInsertBlock()));

        for ( auto op : c.op_integers )
            switch_->addCase(op, b->GetInsertBlock());
    }

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

llvm::Value* CodeGen::llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, llvm::Value* default_, StructGetCallBack* cb, const Location& l)
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

    auto notzero = builder()->CreateICmpNE(isset, llvmConstInt(0, 32));
    llvmCreateCondBr(notzero, block_ok, block_not_set);

    pushBuilder(block_ok);

    // Load field
    addr = llvmGEP(sval, zero, llvmGEPIdx(idx + 2));
    llvm::Value* result_ok = builder()->CreateLoad(addr);

    if ( cb )
        result_ok = (*cb)(this, result_ok);

    llvmCreateBr(block_done);
    popBuilder();

    pushBuilder(block_not_set);

    // Unset, raise exception if no default.

    if ( ! default_ )
        llvmRaiseException("hlt_exception_undefined_value", l);

    llvmCreateBr(block_done);
    popBuilder();

    pushBuilder(block_done);

    llvm::Value* result = nullptr;

    if ( default_ ) {
        auto phi = builder()->CreatePHI(result_ok->getType(), 2);
        phi->addIncoming(result_ok, block_ok->GetInsertBlock());
        phi->addIncoming(default_, block_not_set->GetInsertBlock());
        result = phi;
    }

    else
        result = result_ok;

    llvmCctor(result, f->type(), false);

    // Leave builder on stack.

    return result;
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
    llvmGCAssign(addr, val, f->type(), true);
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

llvm::Value* CodeGen::llvmTupleElement(shared_ptr<Type> ttype, llvm::Value* tval, int idx, bool cctor)
{
    assert(ttype || ! cctor);

    // ttype can be null if cctor is false.

    assert(false);
    // Not yet implemented.
}

llvm::Value* CodeGen::llvmIterBytesEnd()
{
    return llvmConstNull(llvmLibType("hlt.iterator.bytes"));
}

#if 0
llvm::Value* CodeGen::llvmMalloc(llvm::Type* ty)
{
    value_list args { llvmSizeOf(ty); }
    auto result = llvmCallC("hlt_malloc", args, false, false);
    return builder()->CreateBitCast(result, llvmTypePtr(ty));
}
#endif

llvm::Value* CodeGen::llvmObjectNew(shared_ptr<Type> type, llvm::StructType* llvm_type)
{
    value_list args { llvmRtti(type), llvmSizeOf(llvm_type), llvmConstAsciizPtr("fill-me-in") };
    auto result = llvmCallC("__hlt_object_new", args, false, false);
    return builder()->CreateBitCast(result, llvmType(type));
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
