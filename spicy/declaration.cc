
#include "declaration.h"
#include "constant.h"
#include "expression.h"
#include "function.h"
#include "id.h"
#include "type.h"
#include "variable.h"

using namespace spicy;

string Declaration::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

Declaration::Declaration(shared_ptr<spicy::ID> id, Linkage linkage, const Location& l)
    : ast::Declaration<AstInfo>(id, linkage, l)
{
}

declaration::Variable::Variable(shared_ptr<spicy::ID> id, Linkage linkage,
                                shared_ptr<spicy::Variable> var, const Location& l)
    : spicy::Declaration(id, linkage, l), ast::declaration::mixin::Variable<AstInfo>(this, var)
{
}


declaration::Constant::Constant(shared_ptr<spicy::ID> id, Linkage linkage,
                                shared_ptr<spicy::Expression> value, const Location& l)
    : spicy::Declaration(id, linkage, l), ast::declaration::mixin::Constant<AstInfo>(this, value)
{
}

declaration::Type::Type(shared_ptr<spicy::ID> id, Linkage linkage, shared_ptr<spicy::Type> type,
                        const Location& l)
    : spicy::Declaration(id, linkage, l), ast::declaration::mixin::Type<AstInfo>(this, type)
{
}

declaration::Function::Function(shared_ptr<spicy::Function> func, Linkage linkage,
                                const Location& l)
    : spicy::Declaration(func->id(), linkage, l),
      ast::declaration::mixin::Function<AstInfo>(this, func)
{
}

declaration::Hook::Hook(shared_ptr<spicy::ID> id, shared_ptr<spicy::Hook> hook, const Location& l)
    : Declaration(id, Declaration::EXPORTED, l)
{
    _hook = hook;
    addChild(_hook);
}

shared_ptr<spicy::Hook> declaration::Hook::hook() const
{
    return _hook;
}
