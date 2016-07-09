
#include "function.h"
#include "hilti-intern.h"

using namespace hilti;

Function::Function(shared_ptr<ID> id, shared_ptr<hilti::type::Function> ftype,
                   shared_ptr<Module> module, shared_ptr<Statement> body, const Location& l)
    : ast::Function<AstInfo>(id, ftype, module, body, l)
{
}

shared_ptr<type::Scope> Function::scope() const
{
    auto t = type()->attributes().getAsType(attribute::SCOPE);

    if ( ! t )
        return nullptr;

    return ast::rtti::checkedCast<type::Scope>(t);
}

Hook::Hook(shared_ptr<ID> id, shared_ptr<hilti::type::Hook> ftype, shared_ptr<Module> module,
           shared_ptr<Statement> body, const Location& l)
    : Function(id, ftype, module, body, l)
{
}
