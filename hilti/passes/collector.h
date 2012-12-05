
#ifndef HILTI_PASSES_COLLECT_H
#define HILTI_PASSES_COLLECT_H

#include "../pass.h"

namespace hilti {
namespace passes {

// Collects information about an AST for later use. Currently, it collects
// only a list of all declared globals.
class Collector : public Pass<>
{
public:
   /// Constructor.
   Collector() : Pass<>("hilti::codegen::Collector") {}

   /// Collects information about an AST.
   bool run(shared_ptr<hilti::Node> module) override {
       bool result = processAllPreOrder(module);
       has_run = true;
       return result;
   }

   /// Returns true if run() has already been called.
   bool hasRun() const { return has_run; }

   typedef std::list<shared_ptr<Variable>> var_list;

   /// Returns all declared globals sorted by name. Must only be called after run() has run.
   const var_list& globals();

protected:
   virtual void visit(declaration::Variable* v);

private:
   bool has_run = false;
   var_list _globals;
};

}
}

#endif
