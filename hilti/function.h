
#ifndef HILTI_FUNCTION_H
#define HILTI_FUNCTION_H

#include <ast/ast.h>

#include "common.h"
#include "module.h"
#include "type.h"
#include "statement.h"

namespace hilti {

namespace function {

typedef type::function::Parameter Parameter;
typedef type::function::parameter_list parameter_list;

}

/// AST node for a function.
class Function : public ast::Function<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: A non-scoped ID with the function's name.
   ///
   /// module: The module the function is part of. Note that it will not
   /// automatically be added to that module.
   ///
   /// body: A statement with the function's body. Typically, the statement type will be that of a block of statements.
   ///
   /// l: Location associated with the node.
   Function(shared_ptr<ID> id, shared_ptr<hilti::type::Function> ftype, shared_ptr<Module> module, shared_ptr<Statement> body = nullptr, const Location& l=Location::None)
       : ast::Function<AstInfo>(id, ftype, module, body, l) {}

   ACCEPT_VISITOR_ROOT()
};

}

#endif
