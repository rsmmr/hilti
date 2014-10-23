
#ifndef HILTI_FUNCTION_H
#define HILTI_FUNCTION_H

#include <ast/function.h>

#include "common.h"
#include "module.h"
#include "type.h"
#include "statement.h"

namespace hilti {

namespace function {

typedef type::function::Result Result;
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
   Function(shared_ptr<ID> id, shared_ptr<hilti::type::Function> ftype, shared_ptr<Module> module,
            shared_ptr<Statement> body = nullptr, const Location& l=Location::None);

   /// Returns the function's scheduling scope. Returns null if not set.
   ///
   /// This is a short-cut to querying the function type's \a &scope attribute.
   shared_ptr<type::Scope> scope() const;

   /// Marks this function as an "init" function. Such functions will be
   /// automatically executed at startup and must not be called otherwise.
   /// They must not take any parameters and nor return a value.
   void setInitFunction() { _init = true; }

   /// Returns whether this is an "init" function.Such functions will be
   /// automatically executed at startup and must not be called otherwise.
   /// They must not take any parameters and nor return a value.
   bool initFunction() { return _init; }

   ACCEPT_VISITOR_ROOT();

private:
   bool _init = false;
};

/// AST node for a hook.
class Hook : public Function
{
public:
   /// id: A non-scoped ID with the hook's name.
   ///
   /// module: The module the hook is part of. Note that it will not
   /// automatically be added to that module.
   ///
   /// body: A statement with the hook's body. Typically, the statement type
   /// will be that of a block of statements. body can be null if this is
   /// just a hook prototype.
   ///
   /// l: Location associated with the node.
   Hook(shared_ptr<ID> id, shared_ptr<hilti::type::Hook> ftype, shared_ptr<Module> module,
        shared_ptr<Statement> body = nullptr, const Location& l=Location::None);

   ACCEPT_VISITOR(Function)
};

}

#endif
