
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
   IdResolver() : Pass<>("hilti::IdResolver") {}
   virtual ~IdResolver();

   /// Resolves ID references.
   ///
   /// module: The AST to resolve.
   ///
   /// Returns: True if no errors were encountered.
   bool run(shared_ptr<Node> module) override;

protected:
   void visit(expression::ID* i) override;
   void visit(Declaration* d) override;
   void visit(Function* f) override;
   void visit(type::Unknown* t) override;
   void visit(declaration::Variable* d) override;
   void visit(statement::ForEach* s) override;

private:
   std::set<string> _locals;
};

}
}

#endif
