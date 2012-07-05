
#ifndef HILTI_MODULE_H
#define HILTI_MODULE_H

#include "common.h"
#include "id.h"
#include "function.h"
#include "statement.h"
#include "type.h"

namespace hilti {

/// AST node for a top-level module.
class Module : public ast::Module<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: A non-scoped ID with the module's name.
   ///
   /// path: A file system path associated with the module.
   ///
   /// l: Associated location.
   Module(shared_ptr<ID> id, const string& path = "-", const Location& l=Location::None);

   /// Returns the module's execution context if any has been declared, or
   /// null if not. Note that the result is only defined after the module has
   /// been finalized.
   shared_ptr<type::Context> context() const;

   ACCEPT_VISITOR_ROOT();

   /// Returns any already existing module associated with a given path. We
   /// keep a global map of all module nodes we have instantiated so far,
   /// indexed by their path. This function queries that map.
   ///
   /// \todo This is for avoiding recursive imports, but keeping this global
   /// state isn't great. We should find a different mechanism. We should
   /// then also move this into \a ast.
   static shared_ptr<Module> getExistingModule(const string& path);

private:
   typedef std::map<string, Module*> module_map;
   static module_map _modules;
};

}

#endif
