
#include "function.h"
#include "module.h"
#include "scope.h"
#include "statement.h"
#include "type.h"

using namespace binpac;

Function::Function(shared_ptr<ID> id, shared_ptr<binpac::type::Function> ftype, shared_ptr<Module> module,
                   shared_ptr<binpac::Statement> body, const Location& l)
    : ast::Function<AstInfo>(id, ftype, module, body, l)

{
}

Hook::Hook(shared_ptr<binpac::Statement> body,
           int prio, bool debug, bool foreach, const Location& l)
    : Node(l)
{
    _body = body;
    _prio = prio;
    _debug = debug;
    _foreach = foreach;

    addChild(_body);
}

shared_ptr<statement::Block> Hook::body() const
{
    return ast::checkedCast<statement::Block>(_body);;
}

int Hook::priority() const
{
    return _prio;
}

bool Hook::debug() const
{
    return _debug;
}

bool Hook::foreach() const
{
    return _foreach;
}

shared_ptr<type::Unit> Hook::unit() const
{
    return _unit;
}

void Hook::setUnit(shared_ptr<type::Unit> unit)
{
    _unit = unit;

    if ( _body )
        body()->scope()->setParent(unit->scope());
}
