
#ifndef BINPAC_JIT_H
#define BINPAC_JIT_H

#include <hilti/jit.h>

namespace binpac {

/// JIT class to use with hilti::CompilerJITContext when jitting BinPAC++
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
