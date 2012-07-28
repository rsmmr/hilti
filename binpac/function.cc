
#include "function.h"
#include "module.h"
#include "statement.h"
#include "type.h"

using namespace binpac;

Function::Function(shared_ptr<ID> id, shared_ptr<binpac::type::Function> ftype, shared_ptr<Module> module,
                   shared_ptr<binpac::Statement> body, const Location& l)
    : ast::Function<AstInfo>(id, ftype, module, body, l)

{
}

Hook::Hook(shared_ptr<ID> id, shared_ptr<binpac::type::Hook> ftype, shared_ptr<Module> module,
           shared_ptr<binpac::Statement> body, int prio, bool debug, bool foreach, const Location& l)
    : Function(id, ftype, module, body, l)
{
    _prio = prio;
    _debug = debug;
    _foreach = foreach;
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

