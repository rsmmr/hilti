
#include "constant.h"
#include "declaration.h"
#include "function.h"
#include "id.h"
#include "type.h"
#include "variable.h"
#include "expression.h"

using namespace binpac;

Declaration::Declaration(shared_ptr<binpac::ID> id, Linkage linkage, const Location& l)
    : ast::Declaration<AstInfo>(id, linkage, l)
{
}

declaration::Variable::Variable(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Variable> var, const Location& l)
    : binpac::Declaration(id, linkage, l), ast::declaration::mixin::Variable<AstInfo>(this, var)
{
}


declaration::Constant::Constant(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Expression> value, const Location& l)
    : binpac::Declaration(id, linkage, l), ast::declaration::mixin::Constant<AstInfo>(this, value)
{
}

declaration::Type::Type(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Type> type, const Location& l)
    : binpac::Declaration(id, linkage, l), ast::declaration::mixin::Type<AstInfo>(this, type)
{
}

declaration::Function::Function(shared_ptr<binpac::Function> func, Linkage linkage, const Location& l)
    : binpac::Declaration(func->id(), linkage, l), ast::declaration::mixin::Function<AstInfo>(this, func)
{
}

declaration::Hook::Hook(shared_ptr<binpac::ID> id, shared_ptr<binpac::Hook> hook, const Location& l)
    : Declaration(id, Declaration::EXPORTED, l)
{
    _hook = hook;
    addChild(_hook);
}

shared_ptr<binpac::Hook> declaration::Hook::hook() const
{
    return _hook;
}

