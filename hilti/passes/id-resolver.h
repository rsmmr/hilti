
#ifndef HILTI_ID_RESOLVER_H
#define HILTI_ID_RESOLVER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Resolves ID references. It replaces nodes of type ID with the node that
/// it's referencing.
class IdResolver : public Pass<>
{
public:
   IdResolver() : Pass<>("IdResolver") {}
   virtual ~IdResolver();

   /// Resolves ID references.
   ///
   /// module: The AST to resolve.
   ///
   /// Returns: True if no errors were encountered.
   bool run(shared_ptr<Node> module) override {
       return processAllPostOrder(module);
   }

protected:
   void visit(expression::ID* i) override;
   void visit(Declaration* d) override;
};

}
}

#endif
