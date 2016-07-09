
#ifndef BINPAC_VARIABLE_H
#define BINPAC_VARIABLE_H

#include <ast/variable.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

/// Base class for AST variable nodes.
class Variable : public ast::Variable<AstInfo> {
public:
    /// Constructor.
    ///
    /// id: The name of the variable. Must be non-scoped.
    ///
    /// type: The type of the variable.
    ///
    /// Expression: An optional initialization expression, or null if none.
    ///
    /// l: Associated location.
    Variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr,
             const Location& l = Location::None);

    ACCEPT_VISITOR_ROOT();
};

namespace variable {

/// AST node representing a global variable.
class Global : public binpac::Variable, public ast::variable::mixin::Global<AstInfo> {
public:
    /// Constructor.
    ///
    /// id: The name of the variable. Must be non-scoped.
    ///
    /// type: The type of the variable.
    ///
    /// Expression: An optional initialization expression, or null if none.
    ///
    /// l: Associated location.
    Global(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr,
           const Location& l = Location::None);

    ACCEPT_VISITOR(binpac::Variable);
};

/// AST node representing a local variable.
class Local : public binpac::Variable, public ast::variable::mixin::Local<AstInfo> {
public:
    /// Constructor.
    ///
    /// id: The name of the variable. Must be non-scoped.
    ///
    /// type: The type of the variable.
    ///
    /// Expression: An optional initialization expression, or null if none.
    ///
    /// l: Associated location.
    Local(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr,
          const Location& l = Location::None);

    ACCEPT_VISITOR(binpac::Variable);
};
}
}

#endif
