
#include "hilti.h"
#include "function.h"

using namespace hilti;

Function::Function(shared_ptr<ID> id, shared_ptr<hilti::type::Function> ftype, shared_ptr<Module> module, shared_ptr<Statement> body, const Location& l)
    : ast::Function<AstInfo>(id, ftype, module, body, l)

{
}

Hook::Hook(shared_ptr<ID> id, shared_ptr<hilti::type::Hook> ftype, shared_ptr<Module> module, const attribute_list& attrs, 
           shared_ptr<Statement> body, const Location& l) : Function(id, ftype, module, body, l)
{
    _priority = 0;
    _group = 0;

    for ( auto a : attrs ) {
        if ( a.first == "&priority" )
            _priority = a.second;
        else if ( a.first == "&group" )
            _group = a.second;
        else
            throw std::runtime_error("unknown hook attribute " + a.first);
    }
}
