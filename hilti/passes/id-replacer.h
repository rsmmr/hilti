
#ifndef HILTI_PASSES_ID_REPLACER_H
#define HILTI_PASSES_ID_REPLACER_H

#include "../pass.h"

namespace hilti {
namespace passes {

// Replaces references to an ID/label with a different one in a given
// sub-tree of the AST.
class IDReplacer : public Pass<>
{
public:
   /// Constructor.
   IDReplacer() : Pass<>("hilti::passes::IDReplacer") {}

   /// Collects information about an AST.
   bool run(shared_ptr<Node> node, shared_ptr<ID> old_id, shared_ptr<ID> new_id);

protected:
   bool run(shared_ptr<Node> ast) override;

   void visit(expression::ID* id) override;
   void visit(expression::Constant* label) override;

private:
   shared_ptr<ID> _old_id;
   shared_ptr<ID> _new_id;
};

}
}

#endif
