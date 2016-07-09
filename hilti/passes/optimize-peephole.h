
#ifndef HILTI_PASSES_OPTIMIZE_PEEPHOLE_H
#define HILTI_PASSES_OPTIMIZE_PEEPHOLE_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Bundles a set of small peep-hole optimizations that act locally on
/// individual instructions without needing further global context.
class OptimizePeepHole : public Pass<> {
public:
    OptimizePeepHole();

    bool run(shared_ptr<hilti::Node> module);

protected:
};
}
}

#endif
