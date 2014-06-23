
#include "hilti-intern.h"
#include "function.h"

using namespace hilti;

Function::Function(shared_ptr<ID> id, shared_ptr<hilti::type::Function> ftype, shared_ptr<Module> module, shared_ptr<Type> scope, const attribute_list& attrs, shared_ptr<Statement> body, const Location& l)
    : ast::Function<AstInfo>(id, ftype, module, body, l)

{
    _scope = scope;
     addChild(_scope);

    _noyield = false;

    for ( auto a : attrs ) {
        if ( a.first == "&noyield" )
            _noyield = true;
    }
}

shared_ptr<type::Scope> Function::scope() const
{
    if ( ! _scope )
        return nullptr;

    shared_ptr<Node> n(_scope);
    auto s = ast::as<type::Scope>(n);
    assert(s);
    return s;
}

Hook::Hook(shared_ptr<ID> id, shared_ptr<hilti::type::Hook> ftype, shared_ptr<Module> module, shared_ptr<Type> scope, const attribute_list& attrs, 
           shared_ptr<Statement> body, const Location& l) : Function(id, ftype, module, scope, attrs, body, l)
{
    _priority = 0;
    _group = 0;

    for ( auto a : attrs ) {
        if ( a.first == "&priority" )
            _priority = a.second;
        else if ( a.first == "&group" )
            _group = a.second;
    }
}

