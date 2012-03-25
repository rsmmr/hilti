
#include "hilti.h"
#include "declaration.h"

using namespace hilti;

declaration::Function::Function(shared_ptr<hilti::Function> func, const Location& l)
    : hilti::Declaration(func->id(), l), ast::declaration::mixin::Function<AstInfo>(this, func)
{
}

declaration::Hook::Hook(shared_ptr<hilti::Hook> hook, const Location& l)
    : Function(hook, l)
{
}

shared_ptr<hilti::Hook> declaration::Hook::hook() const
{
    return ast::as<hilti::Hook>(function());
}
