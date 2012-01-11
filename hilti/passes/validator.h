
#ifndef HILTI_VALIDATOR_H
#define HILTI_VALIDATOR_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Verifies the semantic of an AST.
class Validator : public Pass<>
{
public:
   Validator() : Pass<>("Validator") {}
   virtual ~Validator();

   /// Verifies a given module.
   ///
   /// Returns: True if no errors were found.
   bool run(shared_ptr<Node> module) override {
       return processAllPreOrder(module);
   }

protected:
   void visit(Module* m) override;
   void visit(ID* id) override;

   void visit(statement::Block* b) override;
   void visit(statement::instruction::Unresolved* i) override;
   void visit(statement::instruction::Resolved* i) override;

   void visit(declaration::Variable* v) override;

   void visit(type::Integer* i) override;
   void visit(type::String* s) override;

   void visit(expression::Constant* e) override;

   void visit(constant::Integer* i) override;
   void visit(constant::String* s) override;

};

}
}

#endif
