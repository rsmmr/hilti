
#include <hilti.h>

#include "codegen.h"
#include "coercion-builder.h"
#include "code-builder.h"
#include "type-builder.h"

#include "module.h"
#include "statement.h"
#include "expression.h"

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
    _type_builder = unique_ptr<codegen::TypeBuilder>(new codegen::TypeBuilder(this));

    try {
        hilti::path_list paths;
        auto id = hilti::builder::id::node(module->id()->name(), module->id()->location());
        _mbuilder = std::make_shared<hilti::builder::ModuleBuilder>(id, module->path(), paths, module->location());
        _mbuilder->importModule("Hilti");

        _code_builder->processOne(module);

        _mbuilder->finalize(true, verify);

        return _mbuilder->module();
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
    return hilti::builder::id::node(id->name(), id->location());
}
