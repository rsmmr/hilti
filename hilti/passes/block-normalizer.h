
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
   BlockNormalizer(bool instructions_normalized);

   /// Collects information about an AST.
   bool run(shared_ptr<hilti::Node> module);

protected:
   virtual void visit(statement::Block* b);
   virtual void visit(declaration::Function* f);

private:
   bool _instructions_normalized = false;
   uint64_t _anon_cnt = 0;
};

}
}

#endif
