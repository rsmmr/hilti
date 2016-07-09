
#include "../hilti.h"

#include "codegen.h"
#include "debug-info-builder.h"

using namespace hilti;
using namespace codegen;

DebugInfoBuilder::DebugInfoBuilder(CodeGen* cg)
    : CGVisitor<llvm::MDNode*>(cg, "codegen::DebugInfoBuilderInfoBuilder")
{
}

DebugInfoBuilder::~DebugInfoBuilder()
{
}
