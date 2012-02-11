
#ifndef HILTI_PASSES_BLOCK_NORMALIZER_H
#define HILTI_PASSES_BLOCK_NORMALIZER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Normalizes blocks by inserting terminator instructions where missing.
class BlockNormalizer : public Pass<>
{
public:
   /// Constructor.
   BlockNormalizer() : Pass<>("codegen::BlockNormalizer") {}

   /// Collects information about an AST.
   bool run(shared_ptr<hilti::Node> module) override {
       return processAllPreOrder(module);
   }

protected:
   virtual void visit(statement::Block* b);
};

}
}

#endif
