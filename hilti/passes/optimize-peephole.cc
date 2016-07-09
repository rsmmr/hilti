
#include "../builder/nodes.h"
#include "../module.h"
#include "../statement.h"

#include "hilti/autogen/instructions.h"
#include "optimize-peephole.h"
#include "printer.h"

using namespace hilti;
using namespace passes;

OptimizePeepHole::OptimizePeepHole() : Pass<>("hilti::OptimizePeepHole", true)
{
}

bool OptimizePeepHole::run(shared_ptr<hilti::Node> module)
{
    return processAllPreOrder(module);
}
