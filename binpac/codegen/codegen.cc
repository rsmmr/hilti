
#include <hilti.h>

#include "codegen.h"

#include "code-builder.h"
#include "coercion-builder.h"
#include "parser-builder.h"
#include "type-builder.h"
#include "context.h"

#include "module.h"
#include "statement.h"
#include "expression.h"
#include "grammar.h"

using namespace binpac;

CodeGen::CodeGen()
{
}

CodeGen::~CodeGen()
{

}

shared_ptr<hilti::Module> CodeGen::compile(shared_ptr<Module> module, int debug, bool verify)
{
    _compiling = true;
    _debug = debug;
    _verify = verify;

    _code_builder = unique_ptr<codegen::CodeBuilder>(new codegen::CodeBuilder(this));
    _coercion_builder = unique_ptr<codegen::CoercionBuilder>(new codegen::CoercionBuilder(this));
    _parser_builder = unique_ptr<codegen::ParserBuilder>(new codegen::ParserBuilder(this));
    _type_builder = unique_ptr<codegen::TypeBuilder>(new codegen::TypeBuilder(this));

    try {
        hilti::path_list paths = module->context()->libraryPaths();
        auto id = hilti::builder::id::node(module->id()->name(), module->id()->location());
        auto ctx = module->context()->hiltiContext();
        _mbuilder = std::make_shared<hilti::builder::ModuleBuilder>(ctx, id, module->path(), module->location());
        _mbuilder->importModule("Hilti");
        _mbuilder->importModule("BinPACHilti");

        if ( util::strtolower(module->id()->name()) != "binpac" )
            _mbuilder->importModule("BinPAC");

        _code_builder->processOne(module);

        auto module = _mbuilder->finalize();

        if ( ! module )
            return verify ? nullptr : _mbuilder->module();

        return module;
    }

    catch ( const ast::FatalLoggerError& err ) {
        // Message has already been printed.
        return nullptr;
    }
}

shared_ptr<hilti::builder::ModuleBuilder> CodeGen::moduleBuilder() const
{
    assert(_compiling);

    return _mbuilder;
}

shared_ptr<hilti::builder::BlockBuilder> CodeGen::builder() const
{
    assert(_compiling);

    return _mbuilder->builder();
}

int CodeGen::debugLevel() const
{
    return _debug;
}

shared_ptr<binpac::type::Unit> CodeGen::unit() const
{
    return _parser_builder->unit();
}

shared_ptr<hilti::Expression> CodeGen::hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to)
{
    assert(_compiling);
    return _code_builder->hiltiExpression(expr, coerce_to);
}

void CodeGen::hiltiStatement(shared_ptr<Statement> stmt)
{
    assert(_compiling);
    _code_builder->hiltiStatement(stmt);
}

shared_ptr<hilti::Type> CodeGen::hiltiType(shared_ptr<Type> type)
{
    assert(_compiling);
    return _type_builder->hiltiType(type);
}

shared_ptr<hilti::Expression> CodeGen::hiltiDefault(shared_ptr<Type> type)
{
    assert(_compiling);
    return _type_builder->hiltiDefault(type);
}

shared_ptr<hilti::Expression> CodeGen::hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst)
{
    assert(_compiling);
    return _coercion_builder->hiltiCoerce(expr, src, dst);
}

shared_ptr<hilti::ID> CodeGen::hiltiID(shared_ptr<ID> id)
{
    return hilti::builder::id::node(id->pathAsString(), id->location());
}

shared_ptr<hilti::Expression> CodeGen::hiltiParseFunction(shared_ptr<type::Unit> u)
{
    auto grammar = u->grammar();
    assert(grammar);

    auto func = ast::tryCast<hilti::Expression>(_mbuilder->lookupNode("parse-func", grammar->name()));

    if ( func )
        return func;

    func = _parser_builder->hiltiCreateParseFunction(u);

    _mbuilder->cacheNode("parse-func", grammar->name(), func);
    return func;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObject(shared_ptr<type::Unit> unit)
{
    auto t = ast::tryCast<hilti::Type>(_mbuilder->lookupNode("parse-obj", unit->id()->name()));

    if ( t )
        return t;

    t = _parser_builder->hiltiTypeParseObject(unit);

    _mbuilder->cacheNode("parse-obj", unit->id()->name(), t);
    return t;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObjectRef(shared_ptr<type::Unit> u)
{
    return hilti::builder::reference::type(hiltiTypeParseObject(u));
}

void CodeGen::hiltiExportParser(shared_ptr<type::Unit> unit)
{
    _parser_builder->hiltiExportParser(unit);
}

void CodeGen::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    _parser_builder->hiltiUnitHooks(unit);
}

void CodeGen::hiltiRunFieldHooks(shared_ptr<type::unit::Item> item)
{
    _parser_builder->hiltiRunFieldHooks(item);
}

void CodeGen::hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook)
{
   _parser_builder->hiltiDefineHook(id, hook);
}

shared_ptr<hilti::declaration::Function> CodeGen::hiltiDefineFunction(shared_ptr<ID> id, shared_ptr<Function> func)
{
    auto name = hiltiID(id);
    auto ftype = func->type();
    auto rtype = ftype->result() ? hiltiType(ftype->result()->type()) : hilti::builder::void_::type();
    auto result = hilti::builder::function::result(rtype);

    hilti::builder::function::parameter_list params;

    for ( auto p : ftype->parameters() ) {
        shared_ptr<hilti::Expression> def = p->default_() ? hiltiExpression(p->default_()) : nullptr;
        auto hp = hilti::builder::function::parameter(hiltiID(p->id()), hiltiType(p->type()), p->constant(), def);
        params.push_back(hp);
    }

    auto cookie = hilti::builder::function::parameter("__cookie", hiltiTypeCookie(), false, nullptr);

    hilti::type::function::CallingConvention cc;

    switch ( ftype->callingConvention() ) {
     case type::function::DEFAULT:
        cc = hilti::type::function::HILTI;
        params.push_back(cookie);
        break;

     case type::function::HILTI_C:
        cc = hilti::type::function::HILTI_C;
        params.push_back(cookie);
        break;

     case type::function::C:
        cc = hilti::type::function::C;
        break;

     default:
        internalError("unexpected calling convention in hiltiDefineFunction()");
    }

    if ( func->body() ) {
        auto decl = moduleBuilder()->pushFunction(name, result, params, cc, nullptr, false, func->location());
        hiltiStatement(func->body());
        moduleBuilder()->popFunction();
        return decl;
    }

    else
        return moduleBuilder()->declareFunction(name, result, params, cc, func->location());
}

shared_ptr<hilti::Expression> CodeGen::hiltiSelf()
{
    return _parser_builder->hiltiSelf();
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeCookie()
{
    return hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::UserCookie"));
}

