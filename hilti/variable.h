
#ifndef HILTI_VARIABLE_H
#define HILTI_VARIABLE_H

#include <ast/variable.h>

#include "common.h"

namespace hilti {

/// Base class for AST variable nodes.
class Variable : public ast::Variable<AstInfo>
{
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
   Variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None)
       : ast::Variable<AstInfo>(id, type, init, l) {}

   ACCEPT_VISITOR_ROOT();
};

namespace variable {


/// AST node representing a global variable.
class Global : public hilti::Variable, public ast::variable::mixin::Global<AstInfo>
{
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
   Global(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None)
       : hilti::Variable(id, type, init, l), ast::variable::mixin::Global<AstInfo>(this) {}

   /// Returns true if this global is a true global across all threads. HILTI
   /// doesn't normally have those and this is a hack for testing/evaluation
   /// purposes only. We currently just base this just off the name:
   /// variables starting with __TRUE_GLOBAL_ return true.
   bool isTrueGlobal() const { return ::util::startsWith(id()->name(), "__TRUE_GLOBAL_"); }

   ACCEPT_VISITOR(hilti::Variable);
};

/// AST node representing a local variable.
class Local : public hilti::Variable, public ast::variable::mixin::Local<AstInfo>
{
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
   Local(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None)
       : hilti::Variable(id, type, init, l), ast::variable::mixin::Local<AstInfo>(this) {}

   /// Returns an internal name set by the ID resolver. This name will be
   /// unique across the function the local is defined in (even if there are
   /// other locals of the same name in other blocks.) Returns the ID itself
   /// as long as nobody has set anything.
   string internalName() { return _internal_name.size() ? _internal_name : id()->name(); }

   /// Sets the internal name returned by internalName. This should be called
   /// only by the ID resolver.
   void setInternalName(const string& name) { _internal_name = name; }

   ACCEPT_VISITOR(hilti::Variable);

private:
   string _internal_name = "";
};

}

}

#endif

