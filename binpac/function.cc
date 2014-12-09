
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

shared_ptr<ID> Function::hiltiFunctionID() const
{
    return _hilti_id;
}

void Function::setHiltiFunctionID(shared_ptr<ID> id)
{
    _hilti_id = id;
}

Hook::Hook(shared_ptr<binpac::Statement> body,
           int prio, bool debug, bool foreach, parameter_list args,
           const Location& l)
    : Node(l)
{
    _body = body;
    _prio = prio;
    _debug = debug;
    _foreach = foreach;

    addChild(_body);

    for ( auto a : args )
        _args.push_back(a);

    for ( auto a : _args )
        addChild(a);
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

parameter_list Hook::parameters() const
{
    parameter_list args;

    for ( auto a : _args )
        args.push_back(a);

    return args;
}

shared_ptr<type::Unit> Hook::unit() const
{
    return _unit;
}

void Hook::setUnit(shared_ptr<type::Unit> unit)
{
    _unit = unit;
    body()->scope()->setParent(_unit->scope());
}

void Hook::setDebug()
{
    _debug = true;
}
