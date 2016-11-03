
// TODO: Move this include to ast/mixin.h.
#include <assert.h>

#include "expression.h"
#include "id.h"
#include "type.h"
#include "variable.h"

using namespace spicy;
using namespace spicy::variable;

Variable::Variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init,
                   const Location& l)
    : ast::Variable<AstInfo>(id, type, init, l)
{
}

Global::Global(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init,
               const Location& l)
    : spicy::Variable(id, type, init, l), ast::variable::mixin::Global<AstInfo>(this)
{
}

Local::Local(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init,
             const Location& l)
    : spicy::Variable(id, type, init, l), ast::variable::mixin::Local<AstInfo>(this)
{
}
