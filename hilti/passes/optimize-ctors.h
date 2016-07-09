
#ifndef HILTI_PASSES_OPTIMIZE_CTORS_H
#define HILTI_PASSES_OPTIMIZE_CTORS_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Replaces ctors in constant contexts with a single global version of the
/// value.
class OptimizeCtors : public Pass<> {
public:
    OptimizeCtors();

    bool run(shared_ptr<hilti::Node> module);

protected:
    virtual void visit(expression::Ctor* c);

private:
    shared_ptr<Module> _module;
    static int _id_counter;
};
}
}

#endif
