
#ifndef SPICY_JIT_H
#define SPICY_JIT_H

#include <hilti/jit.h>

namespace spicy {

/// JIT class to use with hilti::CompilerJITContext when jitting Spicy
/// parsers.
class JIT : public hilti::JIT {
public:
    /// Constructor.
    JIT(hilti::CompilerContext* ctx);

    /// Destructor.
    virtual ~JIT();

protected:
    void _jit_init() override;
};
}

#endif
