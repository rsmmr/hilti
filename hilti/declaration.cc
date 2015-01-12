
#include "hilti-intern.h"
#include "declaration.h"

using namespace hilti;

Declaration::Declaration(shared_ptr<hilti::ID> id, const Location& l) : ast::Declaration<AstInfo>(id, LOCAL, l)
{
}

string Declaration::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

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
