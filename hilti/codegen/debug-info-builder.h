
#ifndef HILTI_CODEGEN_DEBUG_INFO_BUILDER_H
#define HILTI_CODEGEN_DEBUG_INFO_BUILDER_H

// This file is not yet used.

#include "../common.h"
#include "../visitor.h"

#include "codegen.h"
#include "common.h"

namespace hilti {
namespace codegen {

class DebugInfoBuilder : public CGVisitor<llvm::MDNode*> {
public:
    DebugInfoBuilder(CodeGen* cg);
    virtual ~DebugInfoBuilder();

protected:
};
}
}

#endif
