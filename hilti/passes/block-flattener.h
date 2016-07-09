
#ifndef HILTI_PASSES_BLOCK_FLATTENER_H
#define HILTI_PASSES_BLOCK_FLATTENER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Flattens nested block structure after all "non-standard" control flow
/// statement have been normalized away.
class BlockFlattener : public Pass<> {
public:
    BlockFlattener();

    bool run(shared_ptr<hilti::Node> module) override;

protected:
    typedef std::list<std::shared_ptr<statement::Block>> block_list;

    shared_ptr<statement::Block> flatten(shared_ptr<statement::Block> src,
                                         shared_ptr<statement::Block> toplevel);

    void visit(declaration::Function* d) override;
    void visit(Module* m) override;
};
}
}

#endif
